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

const double CPUCT = 1.41;

struct MCTSNodeNN {
    GameState state;
    MCTSNodeNN* parent;
    Action action_from_parent;
    std::vector<MCTSNodeNN*> children;

    int visits;
    double score;
    float prior_probability;

    MCTSNodeNN(const GameState& s, MCTSNodeNN* p, Action a, float prob)
        : state(s), parent(p), action_from_parent(a), visits(0), score(0.0), prior_probability(prob) {
    }

    ~MCTSNodeNN() {
        for (MCTSNodeNN* child : children) {
            delete child;
        }
    }

    bool IsTerminal() const { return state.currentState != State::InProgress; }
    bool IsExpanded() const { return !children.empty(); }
};

class MCTS_NN {
public:
    static std::vector<float> RunSearch(GameState& rootState, int iterations, py::object model) {
        MCTSNodeNN* root = new MCTSNodeNN(rootState, nullptr, Action(), 0.0f);

        for (int i = 0; i < iterations; i++) {
            MCTSNodeNN* node = root;

            while (!node->IsTerminal() && node->IsExpanded()) {
                node = SelectBestPUCTChild(node);
            }

            double value = 0.0;
            if (node->IsTerminal()) {
                value = (node->state.currentState == State::AllCured) ? 1.0 : -1.0;
            }
            else {
                py::array_t<float> state_tensor = py::cast(node->state.ToTensor());

                // --- NEW: Generate Valid Mask in C++ ---
                int size = ActionRanges::COUNT;
                ActionList legalMoves;
                node->state.GetFilteredActions(legalMoves);

                auto mask_array = py::array_t<bool>({ size });
                auto buf = mask_array.request();
                bool* ptr = static_cast<bool*>(buf.ptr);
                std::fill(ptr, ptr + size, false);

                for (int m = 0; m < legalMoves.count; m++) {
                    int idx = ActionDecoder::GetIndexFromAction(legalMoves.Get(m));
                    if (idx >= 0 && idx < size) {
                        ptr[idx] = true;
                    }
                }
                // ---------------------------------------

                // Pass the mask array to the python predict function
                py::tuple output = model.attr("predict")(state_tensor, mask_array);

                py::array_t<float> policy_probs = output[0].cast<py::array_t<float>>();
                value = output[1].cast<double>();

                ExpandNode(node, policy_probs);
            }

            Backpropagate(node, value);
        }

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
        node->state.GetFilteredActions(legalActions);

        auto r = policy_probs.unchecked<1>();

        for (int i = 0; i < legalActions.count; i++) {
            Action a = legalActions.Get(i);
            int idx = ActionDecoder::GetIndexFromAction(a);

            float prob = (idx >= 0 && idx < ActionRanges::COUNT) ? r(idx) : 0.0f;

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