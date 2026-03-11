#ifndef EVAL_H
#define EVAL_H

#include "Game.h"

struct Weights {
    float cure_weight = 0.4f;
    float card_progression = 0.01f;
    float station_dist_penalty = 0.01f;
    float outbreak_penalty = 0.25f;
    float hotspot_penalty = 0.05f;
    float cube_pressure = 0.3f;
    float deck_progress_penalty = 0.1f;

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

float CalculateHeuristicScore(const GameState& state, const Weights& weights);

#endif