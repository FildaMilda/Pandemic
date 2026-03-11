#ifndef MCTS_NN_H
#define MCTS_NN_H

#include "Game.h"
#include "ActionDecoder.h"
#include <vector>
#include <cmath>
#include <limits>
#include <algorithm>
#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <pybind11/stl.h>

namespace py = pybind11;

// Hyperparameter for PUCT: Controls the weight of the Neural Network's "Prior"
const double CPUCT = 1.41;

struct MCTSNodeNN {
    GameState state;
    MCTSNodeNN* parent;
    Action action_from_parent;
    std::vector<MCTSNodeNN*> children;

    int visits;
    double score;             // Total value accumulated from backprop
    float prior_probability;  // P(s,a) from the Neural Network Policy head

    MCTSNodeNN(const GameState& s, MCTSNodeNN* p, Action a, float prob)
        : state(s), parent(p), action_from_parent(a), visits(0), score(0.0), prior_probability(prob) {
    }

    ~MCTSNodeNN() {
        for (MCTSNodeNN* child : children) {
            delete child;
        }
    }

    bool IsTerminal() const {
        return state.currentState != State::InProgress;
    }

    bool IsExpanded() const {
        return !children.empty();
    }
};

class MCTS_NN {
public:
    static std::vector<float> RunSearch(GameState& rootState, int iterations, py::object model) {
        // 1. Initialize Root
        // We evaluate the root immediately to get the initial policy/value
        MCTSNodeNN* root = new MCTSNodeNN(rootState, nullptr, Action(), 0.0f);

        for (int i = 0; i < iterations; i++) {
            MCTSNodeNN* node = root;

            // --- STEP 1: SELECT ---
            // Traverse using PUCT until we hit a leaf node
            while (!node->IsTerminal() && node->IsExpanded()) {
                node = SelectBestPUCTChild(node);
            }

            // --- STEP 2: EXPAND & EVALUATE ---
            double value = 0.0;
            if (node->IsTerminal()) {
                // If the game ended, use the actual result
                value = (node->state.currentState == State::AllCured) ? 1.0 : -1.0;
            }
            else {
                // Otherwise, ask the Neural Network for Policy and Value
                py::array_t<float> state_tensor = py::cast(node->state.ToTensor());
                py::tuple output = model.attr("predict")(state_tensor);

                py::array_t<float> policy_probs = output[0].cast<py::array_t<float>>();
                value = output[1].cast<double>();

                ExpandNode(node, policy_probs);
            }

            // --- STEP 3: BACKPROPAGATE ---
            Backpropagate(node, value);
        }

        // --- STEP 4: PREPARE PROBABILITIES FOR PYTHON ---
        // Target probabilities for training are based on visit counts (N), not scores
        uint64_t sum_of_child_visits = 0;
        for (MCTSNodeNN* child : root->children) {
            sum_of_child_visits += child->visits;
        }

        std::vector<float> pi(ActionRanges::COUNT, 0.0f);
        for (MCTSNodeNN* child : root->children) {
            int idx = ActionDecoder::GetIndexFromAction(child->action_from_parent);
            if (idx >= 0 && idx < ActionRanges::COUNT) {
                pi[idx] += (float)child->visits / (float)sum_of_child_visits;
            }
        }

        delete root;
        return pi;
    }

private:
    static MCTSNodeNN* SelectBestPUCTChild(MCTSNodeNN* node) {
        MCTSNodeNN* bestChild = nullptr;
        double bestValue = -std::numeric_limits<double>::max();

        for (MCTSNodeNN* child : node->children) {
            // PUCT Formula: Q(s,a) + U(s,a)
            // Q = Average value of the node
            // U = Exploration bonus scaled by the Prior Probability from the NN
            double q_value = (child->visits > 0) ? (child->score / child->visits) : 0.0;
            double u_value = CPUCT * child->prior_probability * std::sqrt(node->visits) / (1.0 + child->visits);

            if (q_value + u_value > bestValue) {
                bestValue = q_value + u_value;
                bestChild = child;
            }
        }
        return bestChild;
    }

    static void ExpandNode(MCTSNodeNN* node, py::array_t<float>& policy_probs) {
        ActionList legalActions;
        node->state.GetPossibleActions(legalActions);

        auto r = policy_probs.unchecked<1>();

        for (int i = 0; i < legalActions.count; i++) {
            Action a = legalActions.Get(i);
            int idx = ActionDecoder::GetIndexFromAction(a);

            // Extract the NN's prior for this specific action
            float prob = (idx >= 0 && idx < 1024) ? r(idx) : 0.0f;

            GameState nextState = node->state;
            nextState.Execute(a);

            node->children.push_back(new MCTSNodeNN(nextState, node, a, prob));
        }
    }

    static void Backpropagate(MCTSNodeNN* node, double value) {
        while (node != nullptr) {
            node->visits++;
            node->score += value;
            node = node->parent;
        }
    }
};

#endif