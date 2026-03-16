#include "Eval.h"

/*
float CalculateHeuristicScore(const GameState& state, const Weights& weights)
{
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
        if (res.color == ColorType::NO_COLOR) continue;

        if (!state.gameFlags.IsCured(res.color)) {
            score += (res.count * res.count) * weights.card_progression;

            int threshold = (state.players.GetRole(i) == Role::Scientist) ? CURE_CARD_COUNT - 1 : CURE_CARD_COUNT;
            if (res.count >= threshold) {
                int dist = state.cityState.GetDistanceToNearestStation(state.players.GetLocation(i));
                score += (1.0 / (dist + 1.0)) * weights.station_dist_penalty;
            }
        }
    }

    // Go towards hotspots
    // TODO:

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

    double final_score = std::tanh(score * 0.5);
    return std::clamp(final_score, -0.95, 0.95);
}
*/

float CalculateHeuristicScore(const GameState& state, const Weights& weights)
{
    if (state.currentState == State::AllCured) return 1.0;

    double score = 0.0;
    uint8_t activePlayer = state.gameFlags.GetActivePlayer();
    uint8_t activePlayerLocation = state.players.GetLocation(activePlayer);

    // ===== Positive =====

    // Cured diseases are huge progress
    score += state.gameFlags.GetCuredCount() * weights.cure_weight;

    // Evaluate the team's progress on each uncured color, rather than each player's hand
    for (int c = 0; c < ColorType::COUNT; ++c) {
        ColorType color = (ColorType)c;

        if (state.gameFlags.IsCured(color)) continue;

        int bestCardCount = 0;
        int bestPlayerId = -1;

        // Find the player with the most cards of this color
        for (int p = 0; p < state.players.count; ++p) {
            int count = state.players.GetColorCount(p, color);
            if (count > bestCardCount) {
                bestCardCount = count;
                bestPlayerId = p;
            }
        }

        // Only reward the highest progression for this specific disease
        if (bestCardCount > 0) {
            score += (bestCardCount * bestCardCount) * weights.card_progression;

            int threshold = (state.players.GetRole(bestPlayerId) == Role::Scientist) ? CURE_CARD_COUNT - 1 : CURE_CARD_COUNT;
            if (bestCardCount >= threshold) {
                int dist = state.cityState.GetDistanceToNearestStation(state.players.GetLocation(bestPlayerId));
                score += (1.0 / (dist + 1.0)) * weights.station_dist_reward;
            }
        }
    }

    // Reward the shortest distance from *any* player to *each* hotspot
    uint64_t hotspots = state.cityState.GetHotspotMask();
    for (int cityId = 0; cityId < NUMBER_OF_CITIES; ++cityId) {
        if ((hotspots >> cityId) & 1ULL) {

            int minDistanceToHotspot = 999; // Arbitrarily large number

            // Find the closest player to THIS specific hotspot
            for (int p = 0; p < state.players.count; ++p) {
                int dist = MapData::GetDistance(state.players.GetLocation(p), cityId);
                if (dist < minDistanceToHotspot) {
                    minDistanceToHotspot = dist;
                }
            }

            // Add a reward based on the closest player
            // Multiple players near the same hotspot won't stack the reward
            score += (1.0 / (minDistanceToHotspot + 1.0)) * weights.hotspot_approach_weight;
        }
    }

    // Station Network Bonus (Mobility)
    // Shuttle Flight is very effective
    score += state.cityState.GetStationCount() * weights.station_network_weight;

    // Maybe rewarding Eradication?
    // Add role specific rewards

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

    // Penalize player deck (acts as a timer/urgency mechanic)
    score -= (NUMBER_OF_UNIQUE_CARDS - state.decks.player_deck.Count()) / (float)NUMBER_OF_UNIQUE_CARDS * weights.deck_progress_penalty;

    // Penalize the sheer existence of hotspots
    score -= state.cityState.GetHotspotCount() * weights.hotspot_penalty;

    // Chain Outbreak Risk
    // Penalizing 2 or more neighboring hotspots
    int chainRiskCount = 0;

    for (int cityId = 0; cityId < NUMBER_OF_CITIES; ++cityId) {
        if ((hotspots >> cityId) & 1ULL) {
            // Iterate through neighbors of this hotspot
            uint64_t neighbors = MapData::GetNeighborsMask(cityId);

            // Bitwise AND the neighbors with the hotspots to find adjacent 3-cube cities
            if (hotspots & neighbors) {
                chainRiskCount++;
            }
        }
    }
    score -= chainRiskCount * weights.chain_reaction_penalty;

    double final_score = std::tanh(score * 0.5);
    return std::clamp(final_score, -0.95, 0.95);
}