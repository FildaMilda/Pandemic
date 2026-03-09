#ifndef MCTS_H
#define MCTS_H

#include "Game.h"

#include <vector>
#include <cmath>
#include <limits>
#include <iostream>
#include <algorithm>
#include <cstdint>

struct Weights {
    float cure_weight = 0.4f;
    float card_progression = 0.01f;
    float station_dist_penalty = 0.01f;
    float outbreak_penalty = 1.0f;
    float hotspot_penalty = 0.05f;
    float cube_pressure = 1.5f;
    float deck_progress_penalty = 1.0f;

    void Randomize(std::mt19937& rng) {
        std::uniform_real_distribution<float> dist(0.0f, 3.0f);
        cure_weight = dist(rng);
        card_progression = dist(rng);
        station_dist_penalty = dist(rng);
        outbreak_penalty = dist(rng);
        hotspot_penalty = dist(rng);
        cube_pressure = dist(rng);
        deck_progress_penalty = dist(rng);
    }

    // Mutate weights slightly
    inline void Mutate(float rate, std::mt19937& rng) {
        std::uniform_real_distribution<float> dist(-rate, rate);
        cure_weight += dist(rng);
        card_progression += dist(rng);
        outbreak_penalty += dist(rng);
        hotspot_penalty += dist(rng);
        cube_pressure += dist(rng);
        deck_progress_penalty += dist(rng);
        station_dist_penalty += dist(rng);

        // Ensure weights stay positive/sensible
        // cure_weight = std::max(0.01f, cure_weight);
    }

    inline void Print() const {
        std::cout << "Cure: " << cure_weight << "\nCard: " << card_progression << "\nOutbreak: " << outbreak_penalty << "\nHotspot: " << hotspot_penalty << "\nCube: " << cube_pressure << "\nStation dist: " << station_dist_penalty << "\nDeck progress: " << deck_progress_penalty << "\n";
    }
};

// UCT Constant: controls exploration vs exploitation. 
// 1.41 (sqrt(2)) is standard. Higher = more exploration. 1.41421356
const double UCT_C = 1.41421356;

struct MCTSNode {
    GameState state;                // The snapshot of the game at this node
    MCTSNode* parent;               // Pointer up the tree
    Action action_from_parent;      // The move that led to this state

    std::vector<MCTSNode*> children;
    ActionList untried_actions;     // Moves we haven't tried yet from this state

    int visits;
    double score;                   // Cumulative score (Win = 1, Loss = 0)
    int player_just_moved;          // Who made the move to get here

    // Constructor
    MCTSNode(const GameState& s, MCTSNode* p, Action a)
        : state(s), parent(p), action_from_parent(a), visits(0), score(0.0)
    {
        player_just_moved = (p == nullptr) ? -1 : p->state.gameFlags.GetActivePlayer();

        // Generate all legal moves immediately so we know what is untried
        state.GetFilteredActions(untried_actions);
    }

    // Destructor to recursively clean up memory
    ~MCTSNode() {
        for (MCTSNode* child : children) {
            delete child;
        }
    }

    // Check if this node is fully expanded (all moves have been tried)
    bool IsFullyExpanded() const {
        return untried_actions.count == 0;
    }

    // Check if this is a terminal node (Game Over)
    bool IsTerminal() const {
        return state.currentState != State::InProgress;
    }
};

class MCTS {
public:
    // THE MAIN FUNCTION: Returns the best action after N iterations
    static Action GetBestMove(const GameState& rootState, int iterations, const Weights& weights) {
        // 1. Create the root node
        // Action() is a dummy "no-op" action for the root
        MCTSNode* root = new MCTSNode(rootState, nullptr, Action());

        // 2. The Main Loop
        for (int i = 0; i < iterations; i++) {
            MCTSNode* leaf = Select(root);
            MCTSNode* child = Expand(leaf);
            double result = Simulate(child, weights);
            Backpropagate(child, result);
        }

        /*
        std::cout << "\n--- MCTS Stats (" << iterations << " iterations) ---\n";
        for (MCTSNode* child : root->children) {
            double score = (child->visits > 0) ? (child->score / child->visits) : 0.0;

            std::cout << "Move: ";
            child->action_from_parent.Print();
            std::cout << " | Visits: " << child->visits
                << " | Score: " << (score) << "\n";
        }
        std::cout << "--------------------------------\n";
        */

        // 3. Select best child (Robust Child: most visits)
        // In rigorous MCTS, you pick most visits, not highest score.
        MCTSNode* bestChild = nullptr;
        int maxVisits = -1;

        for (MCTSNode* child : root->children) {
            if (child->visits > maxVisits) {
                maxVisits = child->visits;
                bestChild = child;
            }
        }

        // 4. Extract action and cleanup
        Action bestAction = bestChild->action_from_parent;

        // IMPORTANT: Detach the best child's subtree if you wanted to keep it (Re-use tree), 
        // but for now we just delete the whole tree to be safe and simple.
        delete root;

        return bestAction;
    }

private:
    // --- STEP 1: SELECT ---
    // Traverse down the tree using UCT until we find a node that isn't fully expanded
    static MCTSNode* Select(MCTSNode* node) {
        while (!node->IsTerminal() && node->IsFullyExpanded()) {
            node = GetBestUCTChild(node);
        }
        return node;
    }

    // Upper Confidence Bound for Trees (UCT) formula
    static MCTSNode* GetBestUCTChild(MCTSNode* node) {
        MCTSNode* bestChild = nullptr;
        double bestValue = -std::numeric_limits<double>::max();
        double logTotalVisits = std::log(node->visits);

        for (MCTSNode* child : node->children) {
            // UCT = (w/n) + c * sqrt(ln(N)/n)
            double uctValue = (child->score / child->visits) +
                UCT_C * std::sqrt(logTotalVisits / child->visits);

            if (uctValue > bestValue) {
                bestValue = uctValue;
                bestChild = child;
            }
        }
        return bestChild;
    }

    // --- STEP 2: EXPAND ---
    // Pick one untried action, create a new node for it, and return that node
    static MCTSNode* Expand(MCTSNode* node) {
        if (node->IsTerminal()) return node;

        // 1. Pick a random untried action
        // (We simply take the last one to avoid shifting the array - O(1) removal)
        int index = node->untried_actions.count - 1;
        Action action = node->untried_actions.Get(index);
        node->untried_actions.count--; // "Remove" it

        // 2. Create the new state
        GameState newState = node->state;
        newState.Execute(action);

        // 3. Create the child node
        MCTSNode* child = new MCTSNode(newState, node, action);
        node->children.push_back(child);

        return child;
    }

    // --- STEP 3: SIMULATE (Rollout) ---
    // Play random moves until the game ends or depth limit is reached
    static double Simulate(MCTSNode* node, const Weights& weights) {
        GameState tempState = node->state;
        ActionList moves;
        int depth = 0;
        const int MAX_DEPTH = 100;

        thread_local std::mt19937 tls_rng(std::random_device{}());

        while (tempState.currentState == State::InProgress && depth < MAX_DEPTH) {
            tempState.GetFilteredActions(moves);

            if (moves.count == 0) break; // Stalemate / Stuck

            // Optimized Random Choice
            std::uniform_int_distribution<int> dist(0, moves.count - 1);
            // Note: In real engine, pass the RNG pointer down!
            // Using a local dirty RNG for brevity here, but fix this in prod!
            //thread_local std::mt19937 rng(std::random_device{}());

            Action randomMove = moves.Get(dist(tls_rng));
            tempState.Execute(randomMove); 
            depth++;
        }

        return EvaluateState(tempState, weights);
    }

    // Heuristic Evaluation
    static double EvaluateState(const GameState& state, const Weights& weights) {
        if (state.currentState == State::AllCured) return 1.0;

        double score = 0.0;
        uint8_t activePlayer = state.gameFlags.GetActivePlayer();
        uint8_t activePlayerLocation = state.players.GetLocation(activePlayer);

        // ===== Positive =====
        // Cured diseases are huge progress
        score += state.gameFlags.GetCuredCount() * weights.cure_weight;

        // Its better if players has 4 of the same colored cards
        // than if he has 4 different colored cards (because of the cure discover)
        for (int i = 0; i < state.players.count; ++i) {
            ColorCount res = state.players.GetMostFrequentColor(i);
            if (!state.gameFlags.IsCured(res.color)) {
                score += (res.count * res.count) * weights.card_progression;

                int threshold = (state.players.GetRole(i) == Role::Scientist) ? CURE_CARD_COUNT-1 : CURE_CARD_COUNT;
                if (res.count >= threshold) {
                    int dist = state.cityState.GetDistanceToNearestStation(state.players.GetLocation(i));
                    score += (1.0 / (dist + 1.0)) * weights.station_dist_penalty;
                }
            }
        }

        // Go towards hotspots


        // ===== Negative =====
        // Penalize outbreaks
        score -= std::pow(state.gameFlags.GetOutbreaks() / 8.0, 3) * weights.outbreak_penalty;

        // Penalize cubes
        float cubePressure = 0;
        for (int c = 0; c < 4; ++c) {
            float count = state.cityState.GetTotalCubeCount((ColorType)c);
            cubePressure += std::pow(count / MAX_NUMBER_OF_CUBES_PER_COLOR, 2);
        }
        score -= (cubePressure / 4.0) * weights.cube_pressure;

        // Penalize player deck
        score -= (NUMBER_OF_UNIQUE_CARDS - state.decks.player_deck.Count()) / NUMBER_OF_UNIQUE_CARDS * weights.deck_progress_penalty;

        // Penalize hotspots
        score -= state.cityState.GetHotspotCount() * weights.hotspot_penalty;

        return std::clamp(score, -1.0, 1.0);
    }

    // --- STEP 4: BACKPROPAGATE ---
    // Walk back up the tree updating visits and scores
    static void Backpropagate(MCTSNode* node, double result) {
        while (node != nullptr) {
            node->visits++;
            node->score += result;
            node = node->parent;
        }
    }
};

#endif
