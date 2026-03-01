import gymnasium as gym
import numpy as np
import pandemic_cpp
from sb3_contrib import MaskablePPO
from sb3_contrib.common.maskable.policies import MaskableActorCriticPolicy
from sb3_contrib.common.wrappers import ActionMasker
from sb3_contrib.common.maskable.utils import get_action_masks

# 1. Define the Gym Environment Wrapper
class PandemicGymEnv(gym.Env):
    def __init__(self):
        super().__init__()
        # Initialize C++ Engine
        self.engine = pandemic_cpp.PandemicEnv(42)
        
        # Define Action Space (0 to 323)
        self.action_space = gym.spaces.Discrete(324)
        
        # Define Observation Space (The 783 floats)
        self.observation_space = gym.spaces.Box(
            low=0.0, high=1.0, shape=(783,), dtype=np.float32
        )

    def reset(self, seed=None, options=None):
        super().reset(seed=seed)
        # Reset C++ Engine
        obs = self.engine.reset()
        return obs, {}

    def step(self, action):
        # Step C++ Engine
        reward, done = self.engine.step(action)
        
        # Get Next Observation
        obs = self.engine.get_tensor()
        
        # Gymnasium requires specific return format
        terminated = done
        truncated = False # We don't use time limits yet
        info = {}
        
        return obs, reward, terminated, truncated, info

    def action_masks(self):
        # Allow the AI to see which moves are legal
        return self.engine.get_valid_mask()

# 2. Boilerplate to make masking work
def mask_fn(env: gym.Env) -> np.ndarray:
    return env.action_masks()

# 3. The Main Training Loop
if __name__ == "__main__":
    print("Setting up Training Environment...")
    
    # Create the environment
    env = PandemicGymEnv()
    
    # Wrap it so the Agent can see the masks
    env = ActionMasker(env, mask_fn)

    # Initialize the Agent (Maskable PPO)
    # verbose=1 prints progress to console
    # device="cuda" forces it to use your RTX 3070
    model = MaskablePPO(
        MaskableActorCriticPolicy, 
        env, 
        verbose=1, 
        device="cuda",
        learning_rate=3e-4,
        gamma=0.99,            # Discount factor (care about future rewards)
        n_steps=2048,          # Steps per update
        batch_size=64
    )

    print("Starting Training on RTX 3070... (Press Ctrl+C to stop)")
    
    # Train for 1,000,000 steps (Change this number as needed)
    try:
        model.learn(total_timesteps=1_000_000, progress_bar=True)
    except KeyboardInterrupt:
        print("Training stopped manually.")

    # Save the brain
    model.save("pandemic_ppo_model")
    print("Model saved to pandemic_ppo_model.zip")