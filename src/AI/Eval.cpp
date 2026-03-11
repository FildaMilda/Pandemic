#include "Eval.h"

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

    return std::clamp(score, -1.0, 1.0);
}
