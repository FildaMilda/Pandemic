#include "Game.h"
#include "Eval.h"

#include <vector>
#include <cmath>
#include <random>
#include <algorithm>

struct MacroMCTSNode {
    int parent_id;  
    int first_child_id;
    int next_sibling_id;

    Turn move_taken;        // The macro that created this state
    TurnList untried_turns; // Remaining macros to explore

    int visits;
    double total_score;     
    double max_score;       // Track max score for cooperative search

    // Fast constructor
    MacroMCTSNode(int parent, const Turn& move, const GameState& state)
        : parent_id(parent), first_child_id(-1), next_sibling_id(-1),
        move_taken(move), visits(0), total_score(0.0), max_score(-9999999.0)
    {
        state.GetPolicyTurns(untried_turns);
    }
};

class MacroMCTS {
private:
    std::vector<MacroMCTSNode> arena;
    const double UCT_CONSTANT = 1.41421356; // sqrt(2)
    int max_rollout_depth = 25;

    // Fast random number generator
    std::mt19937 rng;

public:
    MacroMCTS() {
        // Pre-allocate memory for 1 Million nodes to prevent vector reallocation
        arena.reserve(10000); // reduced from 1M to 10k to prevent 17GB allocation
        rng.seed(1337); // Seed it properly in production
    }

    // Returns the best macro to take from the root state
    Turn Search(const GameState& root_state, int iterations, const Weights& weights = Weights()) {
        arena.clear();

        // Create the root node
        arena.emplace_back(-1, Turn(), root_state);

        for (int i = 0; i < iterations; i++) {
            int node_id = 0; // Start at root
            GameState state = root_state;

            // ==========================================
            // 1. SELECTION (Traverse down the tree)
            // ==========================================
            while (arena[node_id].untried_turns.count == 0 && arena[node_id].first_child_id != -1) {
                node_id = SelectBestChildUCB(node_id);
                state.Execute(arena[node_id].move_taken);
            }

            // ==========================================
            // 2. EXPANSION (Add a new node)
            // ==========================================
            if (arena[node_id].untried_turns.count > 0 && state.currentState == State::InProgress) {
                // Pop the last untried turn
                Turn action = arena[node_id].untried_turns.macros[--arena[node_id].untried_turns.count];

                state.Execute(action);

                // Create child in the arena
                int child_id = arena.size();
                arena.emplace_back(node_id, action, state);

                // Link it using the First-Child / Next-Sibling pattern
                arena[child_id].next_sibling_id = arena[node_id].first_child_id;
                arena[node_id].first_child_id = child_id;

                node_id = child_id;
            }

            // ==========================================
            // 3. SIMULATION (Random Rollout)
            // ==========================================
            double score = Simulate(state, weights);

            // ==========================================
            // 4. BACKPROPAGATION (Update tree)
            // ==========================================
            while (node_id != -1) {
                arena[node_id].visits++;
                arena[node_id].total_score += score;
                arena[node_id].max_score = std::max(arena[node_id].max_score, score);
                node_id = arena[node_id].parent_id;
            }
        }

        // Return the best child of the root based on most visits (Most robust metric)
        return GetMostVisitedChild(0);
    }

private:
    // UCB1 Formula Implementation tweaked for Stochastic Cooperative Search
    int SelectBestChildUCB(int parent_id) {
        int best_child = -1;
        double best_uct = -9999999.0;
        double log_parent_visits = std::log(arena[parent_id].visits);

        // First pass: Find min and max exploitation values to normalize scores dynamically
        double min_exploit = 9999999.0;
        double max_exploit = -9999999.0;
        int child_id = arena[parent_id].first_child_id;
        
        while (child_id != -1) {
            MacroMCTSNode& child = arena[child_id];
            double avg_score = child.total_score / child.visits;
            // Mix Average and Max to prevent random bad luck from tanking correct branches
            double mixed_score = (avg_score + child.max_score) / 2.0; 
            
            if (mixed_score > max_exploit) max_exploit = mixed_score;
            if (mixed_score < min_exploit) min_exploit = mixed_score;
            
            child_id = child.next_sibling_id;
        }
        
        double exploit_range = max_exploit - min_exploit;
        if (!std::isfinite(exploit_range) || exploit_range < 1e-6) exploit_range = 1.0; // Prevent divide by zero / NaN

        // Second pass: Calculate normalized UCB
        child_id = arena[parent_id].first_child_id;
        while (child_id != -1) {
            MacroMCTSNode& child = arena[child_id];

            double avg_score = child.total_score / child.visits;
            double mixed_score = (avg_score + child.max_score) / 2.0;
            
            // Normalize exploit between 0.0 and 1.0 so UCT_CONSTANT behaves predictably
            double normalized_exploit = (mixed_score - min_exploit) / exploit_range;
            if (!std::isfinite(normalized_exploit)) normalized_exploit = 0.0;
            double explore = UCT_CONSTANT * std::sqrt(log_parent_visits / child.visits);
            double uct_value = normalized_exploit + explore;

            if (std::isfinite(uct_value) && uct_value > best_uct) {
                best_uct = uct_value;
                best_child = child_id;
            }

            child_id = child.next_sibling_id;
        }

        if (best_child == -1 && arena[parent_id].first_child_id != -1) {
            best_child = arena[parent_id].first_child_id;
        }

        return best_child;
    }

    // Evaluates a Pandemic game state for cooperative MCTS
    double Simulate(GameState state, const Weights& weights) {
        TurnList rollout_moves;
        int depth = 0;

        // Play random macros until the game ends or we hit the depth limit
        while (state.currentState == State::InProgress && depth < max_rollout_depth) {
            rollout_moves.Clear();
            state.GetPolicyTurns(rollout_moves);

            if (rollout_moves.count == 0) break; // Trapped/No valid moves

            // Pick a random macro
            std::uniform_int_distribution<int> dist(0, rollout_moves.count - 1);
            Turn random_move = rollout_moves.macros[dist(rng)];

            state.Execute(random_move);
            depth++;
        }

        return CalculateHeuristicScoreNew(state, weights);
    }

    Turn GetMostVisitedChild(int parent_id) {
        int best_child = -1;
        int most_visits = -1;

        int child_id = arena[parent_id].first_child_id;

        if (child_id == -1) {
            return Turn();
        }

        while (child_id != -1) {
            if (arena[child_id].visits > most_visits) {
                most_visits = arena[child_id].visits;
                best_child = child_id;
            }
            child_id = arena[child_id].next_sibling_id;
        }

        return arena[best_child].move_taken;
    }
};