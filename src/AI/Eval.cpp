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
    if (state.currentState == State::AllCured) return std::numeric_limits<float>::infinity();;

    float score = 0.0f;
    uint8_t activePlayer = state.gameFlags.GetActivePlayer();
    uint8_t activePlayerLocation = state.players.GetLocation(activePlayer);

    // ===== Positive =====

    // Cured diseases are huge progress
    // 4 cures = win, therefore if CureCount() == 4 should mean score = 1.0
    score += state.gameFlags.GetCuredCount() * 0.25f * weights.cure_weight;

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
            int threshold = (state.players.GetRole(bestPlayerId) == Role::Scientist) ? CURE_CARD_COUNT - 1 : CURE_CARD_COUNT;

            // Having enough cards fot cure and standing on station should have ~ same score as having 1 cure (0.25)
            // Having all the cards is more difficult than the distance to the station, I will assume it's 90% of the job
            score += ((float)bestCardCount / (float)threshold) * 0.25f * 0.9f * weights.card_progression;

            if (bestCardCount >= threshold) {
                int dist = state.cityState.GetDistanceToNearestStation(state.players.GetLocation(bestPlayerId));
                // Adding the extra 10% only if the player already has all the cards.
                if (dist == 0) score += 0.25f * 0.1f * weights.station_dist_reward;
                else score += 0.25f * 0.1f / (float)dist * weights.station_dist_reward;
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
            // Hotspot could potentionally mean an outbreak and 8 outbreak is game over
            // So by curing a hotspot we potentionally save -1/8 score, so we reward that
            score += (0.125 / (minDistanceToHotspot + 1.0)) * weights.hotspot_approach_weight;
        }
    }

    // Station Network Bonus (Mobility)
    // Shuttle Flight is very effective
    score += state.cityState.GetStationCount() * 0.01 * weights.station_network_weight;

    // Maybe rewarding Eradication?
    // Add role specific rewards

    // ===== Negative =====

    // Penalize outbreaks
    // 8 outbreaks is game over
    score -= state.gameFlags.GetOutbreaks() / 8.0 * weights.outbreak_penalty;

    // Penalize cubes
    int max_count = 0; 
    int total_count = 0;
    for (int c = 0; c < 4; ++c) {
        int count = state.cityState.GetTotalCubeCount((ColorType)c);
		if (count > max_count) max_count = count;
        total_count += count;
    }

    score -= total_count / (MAX_NUMBER_OF_CUBES_PER_COLOR * 4);
    score -= max_count / MAX_NUMBER_OF_CUBES_PER_COLOR;

    // Penalize player deck (acts as a timer/urgency mechanic)
    score -= (NUMBER_OF_UNIQUE_CARDS - state.decks.player_deck.Count()) / (float)NUMBER_OF_UNIQUE_CARDS * weights.deck_progress_penalty;

    // Penalize the sheer existence of hotspots
    score -= state.cityState.GetHotspotCount() * 0.125 * weights.hotspot_penalty;

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
    score -= chainRiskCount * 0.25 * weights.chain_reaction_penalty;

    //double final_score = std::tanh(score * 0.5);
    //return (float)std::clamp(final_score, -0.95, 0.95);

    return score;
}

float CalculateHeuristicScoreNew(const GameState& state, const Weights& weights)
{
    if (state.currentState == State::AllCured) return std::numeric_limits<float>::infinity();

    double score = 0.0;
    uint8_t activePlayer = state.gameFlags.GetActivePlayer();
    uint8_t activePlayerLocation = state.players.GetLocation(activePlayer);

    // ===== Positive =====

    // Cured diseases are huge progress
    // 4 cures = win, therefore if CureCount() == 4 should mean score = 1.0
    score += state.gameFlags.GetCuredCount() * 0.25f * weights.cure_weight;

    // 1. EXPONENTIAL CARD PROGRESSION
    for (int c = 0; c < 4; ++c) {
        ColorType color = (ColorType)c;
        if (state.gameFlags.IsCured(color)) continue;

        uint8_t curerId = state.GetDesignatedCurer(color);
        if (curerId == 255) continue;

        int cardCount = state.players.GetColorCount(curerId, color);
        int threshold = (state.players.GetRole(curerId) == Role::Scientist) ? 4 : 5;

        // Exponential Curve: 0 cards = 0, 1 = 0.01, 2 = 0.03, 3 = 0.08, 4 = 0.18, 5 = 0.25
        float progressionScores[6] = { 0.0f, 0.01f, 0.03f, 0.08f, 0.18f, 0.25f };

        // If Scientist, bump them up a bracket so 4 cards equals maximum value
        int scoreIndex = (threshold == 4 && cardCount > 0) ? cardCount + 1 : cardCount;
        scoreIndex = std::min(scoreIndex, 5); // Safety clamp

        score += progressionScores[scoreIndex] * 0.9f * weights.card_progression;

        // Reward standing on a station if we are ready to cure
        if (cardCount >= threshold) {
            int dist = state.cityState.GetDistanceToNearestStation(state.players.GetLocation(curerId));
            if (dist == 0) {
                score += 0.25f * 0.1f * weights.station_dist_reward;
            }
            else {
                score += (0.25f * 0.1f / (dist + 1.0f)) * weights.station_dist_reward;
            }
        }
    }

    // Reward the shortest distance from *any* player to *each* hotspot
    uint64_t hotspots = state.cityState.GetHotspotMask();
    for (int cityId = 0; cityId < NUMBER_OF_CITIES; ++cityId) {
        if ((hotspots >> cityId) & 1ULL) {

            int minDistanceToHotspot = 999;

            // Find the closest player to THIS specific hotspot
            for (int p = 0; p < state.players.count; ++p) {
                int dist = MapData::GetDistance(state.players.GetLocation(p), cityId);
                if (dist < minDistanceToHotspot) {
                    minDistanceToHotspot = dist;
                }
            }

            // Hotspot could potentionally mean an outbreak and 8 outbreak is game over
            // So by curing a hotspot we potentionally save -1/8 score, so we reward that
            score += (0.125 / (minDistanceToHotspot + 1.0)) * weights.hotspot_approach_weight;
        }
    }

    // Station Network Bonus (Mobility)
    score += state.cityState.GetStationCount() * 0.01 * weights.station_network_weight;


    // ===== Negative =====

    // 2. RENDEZVOUS DISTANCE PENALTY (The "Setting up a Share" Heuristic)
    float rendezvousPenalty = 0.0f;

    for (int p = 0; p < state.players.count; ++p) {
        uint64_t hand = state.players.hands[p];
        Role pRole = state.players.GetRole(p);
        uint8_t pLoc = state.players.GetLocation(p);

        while (hand > 0) {
            uint8_t cardId = std::countr_zero(hand);
            hand &= (hand - 1);

            if (CardRegistry::IsEvent(cardId)) continue;
            ColorType color = CardRegistry::GetColor(cardId);
            if (state.gameFlags.IsCured(color)) continue;

            uint8_t curerId = state.GetDesignatedCurer(color);

            // If the player holding the card is NOT the designated curer, penalize the distance
            if (p != curerId && curerId != 255) {
                uint8_t curerLoc = state.players.GetLocation(curerId);
                Role curerRole = state.players.GetRole(curerId);

                int distToRendezvous = 0;

                // Researchers can share anywhere, so they just need to find each other
                if (pRole == Role::Researcher || curerRole == Role::Researcher) {
                    distToRendezvous = MapData::GetDistance(pLoc, curerLoc);
                }
                // Normal share: Both players must travel to the city matching the Card ID
                else {
                    distToRendezvous = MapData::GetDistance(pLoc, cardId) + MapData::GetDistance(curerLoc, cardId);
                }

                rendezvousPenalty += distToRendezvous * 0.015f;
            }
        }
    }

    score -= rendezvousPenalty * weights.rendezvous_penalty_weight;

    // Penalize outbreaks
    score -= state.gameFlags.GetOutbreaks() / 8.0 * weights.outbreak_penalty;

    // Penalize cubes
    int max_count = 0;
    int total_count = 0;
    for (int c = 0; c < 4; ++c) {
        int count = state.cityState.GetTotalCubeCount((ColorType)c);
        if (count > max_count) max_count = count;
        total_count += count;
    }

    score -= total_count / (MAX_NUMBER_OF_CUBES_PER_COLOR * 4.0);
    score -= max_count / (float)MAX_NUMBER_OF_CUBES_PER_COLOR;

    // Penalize player deck (acts as a timer/urgency mechanic)
    score -= (NUMBER_OF_UNIQUE_CARDS - state.decks.player_deck.Count()) / (float)NUMBER_OF_UNIQUE_CARDS * weights.deck_progress_penalty;

    // Penalize the sheer existence of hotspots
    score -= state.cityState.GetHotspotCount() * 0.125 * weights.hotspot_penalty;

    // Chain Outbreak Risk
    int chainRiskCount = 0;

    for (int cityId = 0; cityId < NUMBER_OF_CITIES; ++cityId) {
        if ((hotspots >> cityId) & 1ULL) {
            uint64_t neighbors = MapData::GetNeighborsMask(cityId);
            if (hotspots & neighbors) {
                chainRiskCount++;
            }
        }
    }
    score -= chainRiskCount * 0.25 * weights.chain_reaction_penalty;

    return (float)score;
}