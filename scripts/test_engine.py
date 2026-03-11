import os
import sys

# Get the directory where THIS script is
path_to_pyd = os.path.dirname(os.path.abspath(__file__))

# EXPLICITLY tell Windows to allow loading DLLs from this folder
if os.name == 'nt':
    os.add_dll_directory(path_to_pyd)

try:
    import pandemic_cpp
    print("SUCCESS: Module imported!")
    env = pandemic_cpp.PandemicEnv(42)
    print("SUCCESS: Env created!")
except ImportError as e:
    print(f"FAILURE: {e}")