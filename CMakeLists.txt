cmake_minimum_required(VERSION 3.14)
project(SuperOptimizedPandemic)

# Use C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# --- FIND DEPENDENCIES ---
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(pybind11 CONFIG REQUIRED)

# --- INCLUDE DIRECTORIES ---
# This allows you to include headers from any folder without messy paths
# e.g., inside AI/MCTS.cpp you can just write #include "GameState.h"
include_directories(
    src/GameLogic 
    src/AI 
    src/Bindings
)

# --- DEFINE SOURCE GROUPS ---
# PRO TIP: We use GLOB to automatically find all .cpp files in these folders.
# This means you don't have to update CMake every time you add a new file!

file(GLOB GAMELOGIC_SOURCES "src/GameLogic/*.cpp")
file(GLOB AI_SOURCES "src/AI/*.cpp")
file(GLOB BINDING_SOURCES "src/Bindings/*.cpp")

# --- TARGET 1: THE C++ EXECUTABLE (For Debugging) ---
# It needs GameLogic + AI + main.cpp (but NOT Python bindings)
add_executable(PandemicGame 
    src/main.cpp 
    ${GAMELOGIC_SOURCES} 
    ${AI_SOURCES}
)

# Optimization flags for Executable
if(MSVC)
    target_compile_options(PandemicGame PRIVATE 
        $<$<CONFIG:Release>:/O2 /arch:AVX2>
        $<$<CONFIG:RelWithDebInfo>:/O2 /arch:AVX2>
    )
else()
    target_compile_options(PandemicGame PRIVATE 
        $<$<CONFIG:Release>:-O3 -march=native>
        $<$<CONFIG:RelWithDebInfo>:-O3 -march=native>
    )
endif()

# --- TARGET 2: THE PYTHON MODULE ---
pybind11_add_module(pandemic_cpp 
    ${BINDING_SOURCES}
    ${GAMELOGIC_SOURCES} 
    ${AI_SOURCES}
)
set_target_properties(pandemic_cpp PROPERTIES OUTPUT_NAME "pandemic_cpp")

# Optimization flags: Only apply AVX2 and O2 in Release mode!
if(MSVC)
    target_compile_options(pandemic_cpp PRIVATE 
        $<$<CONFIG:Release>:/O2 /arch:AVX2>
        $<$<CONFIG:RelWithDebInfo>:/O2 /arch:AVX2>
    )
else()
    target_compile_options(pandemic_cpp PRIVATE 
        $<$<CONFIG:Release>:-O3 -march=native>
        $<$<CONFIG:RelWithDebInfo>:-O3 -march=native>
    )
endif()