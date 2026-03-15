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

# Hyperparameters
BATCH_SIZE = 128
LEARNING_RATE = 0.001
MEMORY_SIZE = 25000
MCTS_ITERATIONS = 100

class Trainer:
    def __init__(self):
        self.model = model.PandemicNet()
        self.optimizer = optim.Adam(self.model.parameters(), lr=LEARNING_RATE)
        self.memory = deque(maxlen=MEMORY_SIZE)

        self.writer = SummaryWriter('runs/pandemic_experiment_1')
        self.total_steps = 0

    def execute_self_play(self, num_games, iteration):
        iteration_results = []
        total_cures = 0
        wins = 0

        for _ in tqdm(range(num_games)):
            seed = random.randint(0, 9999)
            env = pandemic_cpp.PandemicEnv(seed)
            game_data = []
            
            done = False
            while not done:

                state = env.get_tensor()
                mask = env.get_valid_mask()
                
                # 1. Run C++ MCTS (Pass the model's predict function)
                # Note: run_mcts returns visit counts as probabilities
                mcts_probs = env.run_mcts(MCTS_ITERATIONS, self.model)
                
                # 2. Pick action (During training, we sample; during play, we pick max)
                action = np.random.choice(len(mcts_probs), p=mcts_probs)
                reward, done = env.step(action)

                # 3. Store state and MCTS search results
                game_data.append([state, mcts_probs, reward])
            
            info = env.get_info()
            total_cures += info['cured_count']
            if info['status'] == "Win_AllCured":
                wins += 1

            # 4. Final outcome 'reward' is either 1.0 (win) or -1.0 (loss)
            final_result = game_data[-1][2]
            for entry in game_data:
                entry[2] = (entry[2] * 0.4) + (final_result * 0.6)
                self.memory.append(entry)

        win_rate = wins / num_games
        cure_percentage = total_cures / (num_games * 4)
        
        self.writer.add_scalar('Game/Win_Rate', win_rate, iteration)
        self.writer.add_scalar('Game/Cure_Percentage', cure_percentage, iteration)
        print(f"Iteration {iteration}: Win Rate: {win_rate:.2f}, Cure %: {cure_percentage:.2f}")

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
    trainer = Trainer()
    print("Starting Training...")

    for iteration in range(1000):
        # 1. Generate Experience
        print("Playing games...")
        trainer.execute_self_play(num_games=20, iteration=iteration)
        
        # 2. Learn from Experience
        print("Learning from games...")
        if len(trainer.memory) >= BATCH_SIZE:
            for _ in range(20): 
                loss = trainer.train_step()
        
        if iteration % 10 == 0:
            print(f"Iteration {iteration} | Loss: {loss:.4f} | Buffer Size: {len(trainer.memory)}")
            torch.save(trainer.model.state_dict(), f"models/pandemic_v{iteration}.pth")