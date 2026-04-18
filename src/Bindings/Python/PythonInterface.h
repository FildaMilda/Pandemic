#ifndef PYTHON_INTERFACE_H
#define PYTHON_INTERFACE_H

#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>
#include <cstdint>
#include "Game.h" 
#include "Eval.h"
#include "ActionDecoder.h"
#include "MCTS_NN.h"
#include "MacroMCTS.h"

namespace py = pybind11;

class PandemicEnv {
    GameState state;
    std::mt19937 rng;

    Difficulty difficulty;
    int player_count;
    int seed;
    MacroMCTS macro_brain;

public:
    Weights heuristic_weights;

    PandemicEnv(int diff, int players, int seed) : rng(seed), difficulty((Difficulty)diff), player_count(players), seed(seed) {
        state.Setup(difficulty, player_count, &rng);
    }

    PandemicEnv Clone();
    py::array_t<float> Reset();

    // The Step function: Takes an action Index (0-324), returns (Reward, Done)
    // You can also return the next Tensor here if you want.
    std::pair<float, bool> Step(int actionIndex);

    // Valid Mask: Returns a boolean array of legal moves (size 324)
    // Neural Net needs this to mask out illegal moves!
    py::array_t<bool> GetValidMask();

    // THE TENSORIZER: Converts C++ State -> Numpy Array
    // This runs FAST in C++, saving Python from doing millions of lookups.
    py::array_t<float> GetTensor();
    py::array_t<float> RunMCTS(int iterations, py::object model);

    // Macro-MCTS helpers
    py::dict StepMacro(int iterations);
    py::dict PlayMacroGame(int iterations);

    py::dict GetGameInfo();

    float GetScore();
};

#endif