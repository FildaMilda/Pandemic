import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import torch.multiprocessing as mp
import random
from collections import deque
import numpy as np
import os
import queue
import traceback
import time

from torch.utils.tensorboard import SummaryWriter
from tqdm import tqdm

import pandemic_cpp
import model

os.environ['OMP_NUM_THREADS'] = '1'
os.environ['MKL_NUM_THREADS'] = '1'

# Hardware Selection
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# Hyperparameters
BATCH_SIZE = 128
START_LR = 0.0005
MIN_LR = 1e-5
MEMORY_SIZE = 25000
TOTAL_GAMES_TO_PLAY = 20000 
SYNC_INTERVAL = 5 # How often (in games) the GPU updates the CPU actors

# MCTS Scaling
MCTS_MIN = 1000
MCTS_MAX = 1000

# Evaluation
EVAL_INTERVAL = 5000
EVAL_GAMES = 100

def get_dynamic_iterations(games_played, total_games):
    """Linearly scales MCTS iterations based on training progress."""
    progress = min(1.0, games_played / total_games)
    iters = MCTS_MIN + (MCTS_MAX - MCTS_MIN) * progress
    return int(iters)

# --- ASYNC WORKER FUNCTION (ACTOR) ---
def continuous_self_play(worker_id, shared_cpu_model, experience_queue, num_games_target, global_games_counter):
    torch.set_num_threads(1) # Critical to keep this

    try:
        games_played = 0
        while games_played < num_games_target:
            seed = random.randint(0, 999999)
            env = pandemic_cpp.PandemicEnv(seed)
            game_data = []
            
            with global_games_counter.get_lock():
                current_global_games = global_games_counter.value
                
            current_mcts = get_dynamic_iterations(current_global_games, TOTAL_GAMES_TO_PLAY)
            
            done = False
            while not done:
                state = env.get_tensor()
                mask = env.get_valid_mask()
                
                # Use the fast shared model directly!
                mcts_probs = env.run_mcts(current_mcts, shared_cpu_model) 

                # Normalize to prevent np.random.choice from crashing
                mcts_probs = np.array(mcts_probs)
                prob_sum = mcts_probs.sum()
                if prob_sum > 0:
                    mcts_probs = mcts_probs / prob_sum
                else:
                    mcts_probs = mask.astype(float)
                    mcts_probs = mcts_probs / mcts_probs.sum()

                action = np.random.choice(len(mcts_probs), p=mcts_probs)
                reward, done = env.step(action)

                game_data.append([state, mask, mcts_probs, reward])
            
            info = env.get_info()
            cures = info['cured_count']
            is_win = (info['status'] == "Win_AllCured")
            
            final_result = game_data[-1][3]
            processed_data = []
            for entry in game_data:
                entry[3] = (entry[3] * 0.4) + (final_result * 0.6)
                processed_data.append(entry)

            experience_queue.put((processed_data, cures, is_win))
            games_played += 1
            
    except Exception as e:
        print(f"\n[CRITICAL] Worker {worker_id} crashed! Error: {e}")
        import traceback
        traceback.print_exc()

def evaluate_model(model_instance, num_games=EVAL_GAMES):
    """Evaluates the raw network (no MCTS). We run this on the GPU/Learner model."""
    model_instance.eval()
    wins = 0
    total_cures = 0
    
    for _ in range(num_games):
        seed = random.randint(0, 999999)
        env = pandemic_cpp.PandemicEnv(seed)
        done = False
        
        while not done:
            state = env.get_tensor()
            mask = env.get_valid_mask()
            
            with torch.no_grad():
                state_t = torch.FloatTensor(state).unsqueeze(0).to(DEVICE)
                mask_t = torch.BoolTensor(mask).unsqueeze(0).to(DEVICE)
                
                log_probs, _ = model_instance(state_t, valid_mask=mask_t)
                
                # Move back to CPU for numpy operations
                probs = log_probs.squeeze(0).cpu().numpy()
                action = np.argmax(probs)
                
            _, done = env.step(action)
            
        info = env.get_info()
        if info['status'] == "Win_AllCured":
            wins += 1
        total_cures += info['cured_count']
        
    model_instance.train()
    return wins / num_games, total_cures / num_games

class AsyncTrainer:
    def __init__(self):
        # 1. The Actor Model (CPU only, Shared Memory)
        self.shared_cpu_model = model.PandemicNet().cpu()
        self.shared_cpu_model.share_memory() 
        
        # 2. The Learner Model (GPU if available, Local)
        self.learner_model = model.PandemicNet().to(DEVICE)
        self.learner_model.load_state_dict(self.shared_cpu_model.state_dict())

        self.optimizer = optim.Adam(self.learner_model.parameters(), lr=START_LR)
        
        # Scheduler (2 steps per game)
        self.scheduler = optim.lr_scheduler.CosineAnnealingLR(
            self.optimizer, 
            T_max=TOTAL_GAMES_TO_PLAY * 2, 
            eta_min=MIN_LR
        )
        
        self.memory = deque(maxlen=MEMORY_SIZE)
        self.writer = SummaryWriter('runs/pandemic_experiment_async')
        
        self.total_train_steps = 0
        self.total_games_played = 0
        self.total_cures_tracker = 0
        self.wins_tracker = 0

    def sync_models(self):
        """Copies weights from the GPU Learner back to the CPU Actors."""
        self.shared_cpu_model.load_state_dict(self.learner_model.state_dict())

    def train_step(self):
        if len(self.memory) < BATCH_SIZE:
            return 0.0

        batch = random.sample(self.memory, BATCH_SIZE)
        states, masks, target_pis, target_vs = zip(*batch)

        # Move tensors to GPU
        states = torch.FloatTensor(np.array(states)).to(DEVICE)
        masks = torch.BoolTensor(np.array(masks)).to(DEVICE)
        target_pis = torch.FloatTensor(np.array(target_pis)).to(DEVICE)
        target_vs = torch.FloatTensor(np.array(target_vs)).unsqueeze(1).to(DEVICE)

        self.learner_model.train()
        log_pis, vs = self.learner_model(states, valid_mask=masks)

        # Track the "Depressed Agent" metric
        self.writer.add_scalar('Value/Average_Prediction', vs.mean().item(), self.total_train_steps)

        policy_loss = -torch.mean(torch.sum(target_pis * log_pis, dim=1))
        value_loss = F.mse_loss(vs, target_vs)
        total_loss = policy_loss + value_loss

        self.optimizer.zero_grad()
        total_loss.backward()
        self.optimizer.step()
        self.scheduler.step() # Step the LR scheduler
        
        current_lr = self.scheduler.get_last_lr()[0]
        self.writer.add_scalar('Training/Learning_Rate', current_lr, self.total_train_steps)
        self.writer.add_scalar('Loss/Total', total_loss.item(), self.total_train_steps)
        self.writer.add_scalar('Loss/Policy', policy_loss.item(), self.total_train_steps)
        self.writer.add_scalar('Loss/Value', value_loss.item(), self.total_train_steps)
        self.writer.add_scalar('Value/Average_Prediction', vs.mean().item(), self.total_train_steps)
        self.total_train_steps += 1

        return total_loss.item()

    def process_new_game(self, game_data, cures, is_win):
        for entry in game_data:
            self.memory.append(entry)
            
        self.total_games_played += 1
        self.total_cures_tracker += cures
        if is_win:
            self.wins_tracker += 1
            
        if self.total_games_played % 20 == 0:
            win_rate = self.wins_tracker / 20
            cure_percentage = self.total_cures_tracker / (20 * 4) 
            
            self.writer.add_scalar('Game/Win_Rate', win_rate, self.total_games_played)
            self.writer.add_scalar('Game/Cure_Percentage', cure_percentage, self.total_games_played)
            
            self.wins_tracker = 0
            self.total_cures_tracker = 0

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    mp.set_start_method('spawn', force=True)
    os.makedirs("models", exist_ok=True)
    
    print(f"Initializing Trainer on Device: {DEVICE}")
    trainer = AsyncTrainer()
    experience_queue = mp.Queue(maxsize=100) 
    
    global_games_counter = mp.Value('i', 0)
    
    num_workers = max(1, mp.cpu_count() - 1)
    games_per_worker = TOTAL_GAMES_TO_PLAY // num_workers
    
    print(f"Starting Async Training: {num_workers} workers playing {TOTAL_GAMES_TO_PLAY} total games.")

    workers = []
    for i in range(num_workers):
        p = mp.Process(
            target=continuous_self_play, 
            # Pass the CPU model to the workers
            args=(i, trainer.shared_cpu_model, experience_queue, games_per_worker, global_games_counter)
        )
        p.start()
        workers.append(p)

    last_eval_game = 0

    try:
        with tqdm(total=TOTAL_GAMES_TO_PLAY, desc="Games Played") as pbar:
            while trainer.total_games_played < TOTAL_GAMES_TO_PLAY:
                try:
                    game_data, cures, is_win = experience_queue.get(timeout=30)
                    trainer.process_new_game(game_data, cures, is_win)
                    
                    with global_games_counter.get_lock():
                        global_games_counter.value += 1
                        
                    pbar.update(1)
                    
                    while not experience_queue.empty():
                        game_data, cures, is_win = experience_queue.get_nowait()
                        trainer.process_new_game(game_data, cures, is_win)
                        with global_games_counter.get_lock():
                            global_games_counter.value += 1
                        pbar.update(1)

                    # Do training steps on the GPU
                    if len(trainer.memory) >= BATCH_SIZE:
                        for _ in range(2): 
                            loss = trainer.train_step()
                        
                        pbar.set_postfix({
                            'Loss': f"{loss:.4f}", 
                            'Buffer': len(trainer.memory)
                        })
                    
                    # Sync weights from GPU to CPU periodically
                    if trainer.total_games_played % SYNC_INTERVAL == 0:
                        trainer.sync_models()

                    # Periodic Evaluation
                    if trainer.total_games_played - last_eval_game >= EVAL_INTERVAL:
                        eval_win_rate, eval_cures = evaluate_model(trainer.learner_model, EVAL_GAMES)
                        trainer.writer.add_scalar('Eval/Win_Rate', eval_win_rate, trainer.total_games_played)
                        trainer.writer.add_scalar('Eval/Avg_Cures', eval_cures, trainer.total_games_played)
                        last_eval_game = trainer.total_games_played

                    if trainer.total_games_played % 100 == 0:
                        torch.save(trainer.learner_model.state_dict(), f"models/pandemic_async_{trainer.total_games_played}.pth")

                except queue.Empty:
                    print("Waiting for games... (Queue empty)")
                    continue

    except KeyboardInterrupt:
        print("\nTraining interrupted! Shutting down workers...")

    finally:
        for w in workers:
            w.terminate()
            w.join()
        print("Final model saved and workers shut down. Training complete.")
        torch.save(trainer.learner_model.state_dict(), "models/pandemic_final.pth")