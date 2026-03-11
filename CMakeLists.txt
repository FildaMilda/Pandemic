cmake_minimum_required(VERSION 3.14)
project(Pandemic)

# Use C++20 standard
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- FIND DEPENDENCIES ---
find_package(Python COMPONENTS Interpreter Development REQUIRED)
find_package(OpenMP REQUIRED)

# --- INCLUDE DIRECTORIES ---
# This allows you to include headers from any folder without messy paths
# e.g., inside AI/MCTS.cpp you can just write #include "GameState.h"
include_directories(
    src/GameLogic 
    src/AI 
    src/Bindings
)

add_subdirectory(extern/godot-cpp)
add_subdirectory(extern/pybind11)

# --- DEFINE SOURCE GROUPS ---
# PRO TIP: We use GLOB to automatically find all .cpp files in these folders.
# This means you don't have to update CMake every time you add a new file!

file(GLOB GAMELOGIC_SOURCES "src/GameLogic/*.cpp")
file(GLOB AI_SOURCES "src/AI/*.cpp")
file(GLOB PYTHON_BINDINGS "src/Bindings/Python/*.cpp")
file(GLOB GODOT_BINDINGS "src/Bindings/Godot/*.cpp")

# --- TARGET 1: THE C++ EXECUTABLE (For Debugging) ---
# It needs GameLogic + AI + main.cpp (but NOT Python bindings)
add_executable(PandemicGame 
    src/Main.cpp 
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
    ${PYTHON_BINDINGS}
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

# AUTOMATION: Copy the compiled .pyd file to the Python folder
add_custom_command(TARGET pandemic_cpp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
    $<TARGET_FILE:pandemic_cpp>
    "${CMAKE_SOURCE_DIR}/Scripts"
    COMMENT "Deploying Python Module to /Scripts folder..."
)

# --- TARGET 3: THE GODOT GDEXTENSION ---
# We create a SHARED library for Godot to load
add_library(pandemic_godot SHARED
    ${GODOT_BINDINGS}
    ${GAMELOGIC_SOURCES}
    ${AI_SOURCES}
)

# Tell this target where to find Godot headers
target_include_directories(pandemic_godot PRIVATE
    extern/godot-cpp/include
    extern/godot-cpp/gen/include
    extern/godot-cpp/core
    src/GameLogic
    src/AI
    src/Bindings
)

# Link against the godot-cpp library 
# (This handles the heavy lifting of connecting to the engine)
target_link_libraries(pandemic_godot PRIVATE godot-cpp)

# Optimization for Godot Target (Match your existing style)
if(MSVC)
    target_compile_options(pandemic_godot PRIVATE $<$<CONFIG:Release>:/O2 /arch:AVX2>)
else()
    target_compile_options(pandemic_godot PRIVATE $<$<CONFIG:Release>:-O3 -march=native>)
endif()


# Automatically copy the compiled library to the Godot bin folder
add_custom_command(TARGET pandemic_godot POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "${CMAKE_SOURCE_DIR}/godot/pandemic/bin"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:pandemic_godot>
        "${CMAKE_SOURCE_DIR}/godot/pandemic/bin/"
    COMMENT "Moving library to Godot project..."
)