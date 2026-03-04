#ifndef EA_H
#define EA_H

#include "Globals.h"
#include "Assets.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <numeric>

struct Weights {
    float cure_weight = 0.4f;
    float card_progression = 0.01f;
    float outbreak_penalty = 1.0f;
    float hotspot_penalty = 0.05f;
    float cube_pressure = 0.3f;
    float proximity_bonus = 0.05f;

    // Mutate weights slightly
    inline void Mutate(float rate, std::mt19937& rng) {
        std::uniform_real_distribution<float> dist(-rate, rate);
        cure_weight += dist(rng);
        card_progression += dist(rng);
        outbreak_penalty += dist(rng);
        hotspot_penalty += dist(rng);
        cube_pressure += dist(rng);
        proximity_bonus += dist(rng);

        // Ensure weights stay positive/sensible
        cure_weight = std::max(0.01f, cure_weight);
    }

    inline void Print() const {
        std::cout << "Cure: " << cure_weight << "\nCard: " << "\nOutbreak: " << outbreak_penalty << "\nHotspot: " << hotspot_penalty << "\nCube: " << cube_pressure << "\nProximity: " << proximity_bonus << "\n";
    }
};

float EvaluateFitness(const Weights& w, int gamesToPlay);
void EvolveWeights();

#endif