import torch
import torch.nn as nn
import torch.nn.functional as F
import torch.optim as optim
import random
from collections import deque
import numpy as np

from torch.utils.tensorboard import SummaryWriter
from tqdm import tqdm

import pandemic_cpp
import model

import multiprocessing as mp

# Hyperparameters
BATCH_SIZE = 128
LEARNING_RATE = 0.001
MEMORY_SIZE = 50000
MCTS_ITERATIONS = 500

def play_single_game(model_state_dict, seed):
    # Each worker needs to recreate the model and load weights
    # (Sharing the actual model object across processes is complex)
    worker_model = model.PandemicNet()
    worker_model.load_state_dict(model_state_dict)
    worker_model.eval()

    env = pandemic_cpp.PandemicEnv(seed)
    game_data = []
    done = False
    
    while not done:
        state = env.get_tensor()
        # Ensure iteration count is high enough for quality
        mcts_probs = env.run_mcts(MCTS_ITERATIONS, worker_model)
        
        # Renormalize to be safe
        mcts_probs = np.array(mcts_probs, dtype=np.float64)
        mcts_probs /= mcts_probs.sum()

        action = np.random.choice(len(mcts_probs), p=mcts_probs)
        reward, done = env.step(action)
        game_data.append([state, mcts_probs, reward])

    info = env.get_info()
    return game_data, info

class Trainer:
    def __init__(self):
        self.model = model.PandemicNet()
        self.optimizer = optim.Adam(self.model.parameters(), lr=LEARNING_RATE)
        self.memory = deque(maxlen=MEMORY_SIZE)

        self.writer = SummaryWriter('runs/pandemic_experiment_1')
        self.total_steps = 0

    @staticmethod
    def _wrap_play_game(args):
        return play_single_game(*args)

    def execute_self_play(self, num_games, iteration):
        # Prepare arguments for the workers
        # We send the state_dict because it's lighter than the whole object
        model_weights = self.model.state_dict()
        seeds = [random.randint(0, 99999) for _ in range(num_games)]
        args = [(model_weights, s) for s in seeds]

        results = []

        try:
            with mp.Pool(processes=4) as pool:
                for result in tqdm(pool.imap_unordered(self._wrap_play_game, args), 
                                   total=num_games, 
                                   desc="Playing Games"):
                    results.append(result)
        except KeyboardInterrupt:
            print("\nCaught KeyboardInterrupt, terminating workers...")
            pool.terminate()
            pool.join()
            exit(1)

        # Process results
        total_cures = 0
        wins = 0
        for game_data, info in results:
            total_cures += info['cured_count']
            if info['status'] == "Win_AllCured":
                wins += 1
            
            # Outcome Backup Logic (70/30 Blend)
            final_outcome = game_data[-1][2]
            for entry in game_data:
                entry[2] = (final_outcome * 0.6) + (entry[2] * 0.4)
                self.memory.append(entry)

        # Logging
        win_rate = wins / num_games
        cure_pct = total_cures / (num_games * 4)
        self.writer.add_scalar('Game/Win_Rate', win_rate, iteration)
        self.writer.add_scalar('Game/Cure_Percentage', cure_pct, iteration)

    def train_step(self):
        if len(self.memory) < BATCH_SIZE:
            return

        # Sample a batch
        batch = random.sample(self.memory, BATCH_SIZE)
        states, target_pis, target_vs = zip(*batch)

        states = torch.FloatTensor(np.array(states))
        target_pis = torch.FloatTensor(np.array(target_pis))
        target_vs = torch.FloatTensor(np.array(target_vs)).unsqueeze(1)

        # Forward Pass
        self.model.train()
        log_pis, vs = self.model(states)

        # Loss Calculation
        # Policy Loss: Cross Entropy between MCTS probs and Model probs
        policy_loss = -torch.mean(torch.sum(target_pis * log_pis, dim=1))
        # Value Loss: Mean Squared Error between Actual result and Predicted result
        value_loss = F.mse_loss(vs, target_vs)
        
        total_loss = policy_loss + value_loss

        # Backprop
        self.optimizer.zero_grad()
        total_loss.backward()
        self.optimizer.step()
        
        self.writer.add_scalar('Loss/Total', total_loss.item(), self.total_steps)
        self.writer.add_scalar('Loss/Policy', policy_loss.item(), self.total_steps)
        self.writer.add_scalar('Loss/Value', value_loss.item(), self.total_steps)
        self.total_steps += 1

        return total_loss.item()

# --- MAIN EXECUTION ---
if __name__ == "__main__":
    mp.set_start_method('spawn', force=True)

    trainer = Trainer()
    print("Starting Training...")

    for iteration in range(1000):
        # 1. Generate Experience
        print("Playing games...")
        trainer.execute_self_play(num_games=100, iteration=iteration)
        
        # 2. Learn from Experience
        print("Learning from games...")
        if len(trainer.memory) >= BATCH_SIZE:
            for _ in range(20): 
                loss = trainer.train_step()
        
        if iteration % 10 == 0:
            print(f"Iteration {iteration} | Loss: {loss:.4f} | Buffer Size: {len(trainer.memory)}")
            torch.save(trainer.model.state_dict(), f"models/pandemic_v{iteration}.pth")