import os
# CRITICAL: Force math libraries to stay single-threaded inside workers
os.environ['OMP_NUM_THREADS'] = '1'
os.environ['MKL_NUM_THREADS'] = '1'

import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import torch.multiprocessing as mp
import random
from collections import deque
import numpy as np
import queue
import time
import gc
import traceback
import gc

from torch.utils.tensorboard import SummaryWriter
from tqdm import tqdm

import pandemic_cpp
import model

# Hardware Selection
DEVICE = torch.device("cuda" if torch.cuda.is_available() else "cpu")

# Hyperparameters
BATCH_SIZE = 128
START_LR = 0.0005
MIN_LR = 1e-5
MEMORY_SIZE = 50000 
TOTAL_GAMES_TO_PLAY = 20000 
SYNC_INTERVAL = 20 # Sync every 20 games to reduce disk overhead

# MCTS Scaling
MCTS_MIN = 100
MCTS_MAX = 5000 # Increased for 100k game run

def get_dynamic_iterations(games_played, total_games):
    progress = min(1.0, games_played / total_games)
    return int(MCTS_MIN + (MCTS_MAX - MCTS_MIN) * progress)

# --- WORKER FUNCTION ---
def continuous_self_play(worker_id, experience_queue, num_games_target, global_games_counter):
    # FORCE each process to stay on its own core
    torch.set_num_threads(1)
    
    # Create a PRIVATE model for this worker
    local_model = model.PandemicNet().cpu()
    local_model.eval() 
    
    try:
        games_played = 0
        while games_played < num_games_target:
            # Sync weights from disk every 10 games
            if games_played % 10 == 0:
                try:
                    local_model.load_state_dict(torch.load("models/latest_weights.pth", weights_only=True))
                except: pass # Skip if file is being written
                
            seed = random.randint(0, 999999)
            env = pandemic_cpp.PandemicEnv(seed)
            game_data = []
            
            with global_games_counter.get_lock():
                current_global_games = global_games_counter.value
            current_mcts = get_dynamic_iterations(current_global_games, TOTAL_GAMES_TO_PLAY)
            
            # --- SPEED BOOST: Disable GC during MCTS ---
            gc.disable() 
            done = False
            while not done:
                state = env.get_tensor()
                mask = env.get_valid_mask()
                mcts_probs = env.run_mcts(current_mcts, local_model) 
                
                # Normalization
                mcts_probs = np.array(mcts_probs)
                mcts_probs /= (mcts_probs.sum() + 1e-8)

                action = np.random.choice(len(mcts_probs), p=mcts_probs)
                reward, done = env.step(action)
                game_data.append([state, mask, mcts_probs, reward])
            
            gc.enable() # Clean up once the game is over
            gc.collect()
            
            info = env.get_info()
            cures, is_win = info['cured_count'], (info['status'] == "Win_AllCured")
            
            final_result = game_data[-1][3]
            processed_data = []
            for entry in game_data:
                entry[3] = (entry[3] * 0.4) + (final_result * 0.6)
                processed_data.append(entry)

            # Put in queue with a long timeout; if it fails, worker might be orphaned
            try:
                experience_queue.put((processed_data, cures, is_win), timeout=60)
            except queue.Full:
                print(f"Worker {worker_id} queue full, skipping game.")

            games_played += 1
            
    except Exception as e:
        print(f"\n[CRITICAL] Worker {worker_id} crashed!")
        traceback.print_exc()

class AsyncTrainer:
    def __init__(self):
        self.learner_model = model.PandemicNet().to(DEVICE)
        self.optimizer = optim.Adam(self.learner_model.parameters(), lr=START_LR)
        self.scheduler = optim.lr_scheduler.CosineAnnealingLR(self.optimizer, T_max=TOTAL_GAMES_TO_PLAY*2, eta_min=MIN_LR)
        self.memory = deque(maxlen=MEMORY_SIZE)
        self.writer = SummaryWriter('runs/pandemic_final_run')
        self.total_train_steps = 0
        self.total_games_played = 0
        self.total_cures_tracker = 0
        self.wins_tracker = 0

    def train_step(self):
        if len(self.memory) < BATCH_SIZE: return 0.0
        batch = random.sample(self.memory, BATCH_SIZE)
        states, masks, target_pis, target_vs = zip(*batch)

        states = torch.FloatTensor(np.array(states)).to(DEVICE)
        masks = torch.BoolTensor(np.array(masks)).to(DEVICE)
        target_pis = torch.FloatTensor(np.array(target_pis)).to(DEVICE)
        target_vs = torch.FloatTensor(np.array(target_vs)).unsqueeze(1).to(DEVICE)

        self.learner_model.train()
        log_pis, vs = self.learner_model(states, valid_mask=masks)
        
        self.writer.add_scalar('Value/Average_Prediction', vs.mean().item(), self.total_train_steps)

        loss = -torch.mean(torch.sum(target_pis * log_pis, dim=1)) + F.mse_loss(vs, target_vs)
        self.optimizer.zero_grad(); loss.backward(); self.optimizer.step(); self.scheduler.step()
        self.total_train_steps += 1
        return loss.item()

# --- MAIN ---
if __name__ == "__main__":
    mp.set_start_method('spawn', force=True)
    os.makedirs("models", exist_ok=True)
    trainer = AsyncTrainer()
    
    # Save initial weights for workers
    torch.save(trainer.learner_model.state_dict(), "models/latest_weights.pth")
    
    experience_queue = mp.Queue(maxsize=100) 
    global_games_counter = mp.Value('i', 0)
    
    # REDUCE worker count slightly to give the OS breathing room
    num_workers = max(1, mp.cpu_count() - 2) 
    
    workers = []
    for i in range(num_workers):
        p = mp.Process(
            target=continuous_self_play,
           args=(i, experience_queue, TOTAL_GAMES_TO_PLAY//num_workers, global_games_counter))
        p.start(); workers.append(p)

    try:
        with tqdm(total=TOTAL_GAMES_TO_PLAY, desc="Games Played") as pbar:
            while trainer.total_games_played < TOTAL_GAMES_TO_PLAY:
                try:
                    game_data, cures, is_win = experience_queue.get(timeout=60)
                    trainer.total_games_played += 1
                    with global_games_counter.get_lock(): global_games_counter.value += 1
                    
                    for entry in game_data: trainer.memory.append(entry)
                    pbar.update(1)

                    if len(trainer.memory) >= BATCH_SIZE:
                        loss = trainer.train_step()
                        pbar.set_postfix({'Loss': f"{loss:.4f}"})
                    
                    if trainer.total_games_played % SYNC_INTERVAL == 0:
                        torch.save(trainer.learner_model.state_dict(), "models/latest_weights.pth")

                except queue.Empty:
                    continue
    finally:
        for w in workers: w.terminate()
        torch.save(trainer.learner_model.state_dict(), "models/pandemic_final.pth")