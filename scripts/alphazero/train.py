import random
from collections import deque

import numpy as np
import torch
import torch.nn.functional as F
import torch.optim as optim
from torch.utils.tensorboard import SummaryWriter
from tqdm import tqdm

import pandemic_cpp
import model

# Hyperparameters
BATCH_SIZE = 128
LEARNING_RATE = 0.001
MEMORY_SIZE = 25000
MACRO_MCTS_ITERATIONS = 1000


class Trainer:
    def __init__(self):
        self.model = model.PandemicNet()
        self.optimizer = optim.Adam(self.model.parameters(), lr=LEARNING_RATE)
        self.memory = deque(maxlen=MEMORY_SIZE)

        self.writer = SummaryWriter('runs/pandemic_macro_experiment_1')
        self.total_steps = 0

    def execute_self_play(self, num_games, iteration):
        total_cures = 0
        wins = 0

        for _ in tqdm(range(num_games)):
            seed = random.randint(0, 9999)
            env = pandemic_cpp.PandemicEnv(0, 4, seed)
            env.weights = pandemic_cpp.Weights()

            game_states = []
            game_result = None

            while True:
                info = env.get_info()
                if info["status"] != "InProgress":
                    game_result = info
                    break

                # Capture the current macro-state before selecting the next turn.
                state = np.array(env.get_tensor(), dtype=np.float32)
                game_states.append(state)

                # One full macro turn chosen by MacroMCTS inside C++.
                step_result = env.step_macro(MACRO_MCTS_ITERATIONS)
                game_result = step_result

                if step_result["done"]:
                    break

            total_cures += int(game_result["cured_count"])
            if game_result["won"]:
                wins += 1

            # Value target: final macro-game outcome.
            target_v = 1.0 if game_result["won"] else -1.0
            for state in game_states:
                self.memory.append((state, target_v))

        win_rate = wins / num_games
        cure_percentage = total_cures / (num_games * 4)

        self.writer.add_scalar('Game/Win_Rate', win_rate, iteration)
        self.writer.add_scalar('Game/Cure_Percentage', cure_percentage, iteration)
        print(f"Iteration {iteration}: Win Rate: {win_rate:.2f}, Cure %: {cure_percentage:.2f}")

    def train_step(self):
        if len(self.memory) < BATCH_SIZE:
            return None

        batch = random.sample(self.memory, BATCH_SIZE)
        states, target_vs = zip(*batch)

        states = torch.FloatTensor(np.array(states))
        target_vs = torch.FloatTensor(np.array(target_vs)).unsqueeze(1)

        self.model.train()
        vs = self.model(states)

        value_loss = F.mse_loss(vs, target_vs)

        self.optimizer.zero_grad()
        value_loss.backward()
        self.optimizer.step()

        self.writer.add_scalar('Loss/Value', value_loss.item(), self.total_steps)
        self.total_steps += 1

        return value_loss.item()


if __name__ == "__main__":
    trainer = Trainer()
    print("Starting Training...")

    for iteration in range(1000):
        print("Playing macro games...")
        trainer.execute_self_play(num_games=20, iteration=iteration)

        print("Learning from games...")
        last_loss = None
        if len(trainer.memory) >= BATCH_SIZE:
            for _ in range(20):
                last_loss = trainer.train_step()

        if iteration % 10 == 0:
            loss_text = f"{last_loss:.4f}" if last_loss is not None else "n/a"
            print(f"Iteration {iteration} | Loss: {loss_text} | Buffer Size: {len(trainer.memory)}")
            torch.save(trainer.model.state_dict(), f"models/pandemic_v{iteration}.pth")