#ifndef EVAL_H
#define EVAL_H

#include "Game.h"

struct Weights {
    float cure_weight = 0.4f;
    float card_progression = 0.01f;
    float station_dist_reward = 0.05f;
    float outbreak_penalty = 0.5f;
    float hotspot_penalty = 0.05f;
    float cube_pressure = 1.0f;
    float deck_progress_penalty = 0.1f;
    float hotspot_approach_weight = 0.1f;
    float station_network_weight = 0.02f;
    float chain_reaction_penalty = 0.1f;

    float researcher_meetup_weight = 0.05f;
    float medic_treat_weight = 0.05f;
    float qs_protect_weight = 0.08f;

    void Randomize(std::mt19937& rng) {
        std::uniform_real_distribution<float> dist(0.0f, 3.0f);
        cure_weight = dist(rng);
        card_progression = dist(rng);
        station_dist_reward = dist(rng);
        outbreak_penalty = dist(rng);
        hotspot_penalty = dist(rng);
        cube_pressure = dist(rng);
        deck_progress_penalty = dist(rng);
        hotspot_approach_weight = dist(rng);
        station_network_weight = dist(rng);
        chain_reaction_penalty = dist(rng);
        researcher_meetup_weight = dist(rng);
        medic_treat_weight = dist(rng);
        qs_protect_weight = dist(rng);

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
        station_dist_reward += dist(rng);
        hotspot_approach_weight += dist(rng);
        station_network_weight += dist(rng);
        chain_reaction_penalty += dist(rng);
        researcher_meetup_weight += dist(rng);
        medic_treat_weight += dist(rng);
        qs_protect_weight += dist(rng);

        // Ensure weights stay positive/sensible
        // cure_weight = std::max(0.01f, cure_weight);
    }

    inline void Print() const {
        std::cout << "Cure: " << cure_weight << "\nCard: " << card_progression << "\nOutbreak: " << outbreak_penalty << "\nHotspot: " << hotspot_penalty << "\nCube: " << cube_pressure << "\nStation dist: " << station_dist_reward << "\nDeck progress: " << deck_progress_penalty << "\nHotspot: " << hotspot_approach_weight << "\nStation network: " << station_network_weight << "\nHotspot chain: " << chain_reaction_penalty << "\n";
    }
};

float CalculateHeuristicScore(const GameState& state, const Weights& weights);

#endif