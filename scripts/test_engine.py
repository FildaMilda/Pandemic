import pandemic_cpp
import sys

print(f"Loaded module: {pandemic_cpp}")

# 2. Check the Environment
# (Pass a seed, e.g., 42, to the constructor)
env = pandemic_cpp.PandemicEnv(42)

# 3. Get the Initial State Tensor
state = env.reset()

print(f"Initial State Tensor Shape: {state.shape}")
print(f"First 10 values: {state}")
print("SUCCESS: C++ Engine is talking to Python!")