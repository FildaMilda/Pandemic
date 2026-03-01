#include "PythonInterface.h"

using namespace pybind11::literals;

py::array_t<float> PandemicEnv::Reset()
{
	state.Setup(&rng);
	return GetTensor();
}

std::pair<float, bool> PandemicEnv::Step(int actionIndex)
{
    float scoreBefore = CalculateHeuristicScore();

    // 2. Execute Action
    Action action = ActionDecoder::GetActionFromIndex(actionIndex, state);
    state.Execute(action);

    // 3. Calculate Score AFTER move
    float scoreAfter = CalculateHeuristicScore();

    // 4. The Reward is the IMPROVEMENT
    float reward = scoreAfter - scoreBefore;

    // 5. Huge Bonus for Winning / Penalty for Losing
    if (state.currentState == State::AllCured) reward += 100.0f;
    if (state.currentState != State::AllCured || state.currentState != State::InProgress) reward -= 10.0f;

    // 6. Tiny time penalty (encourage speed)
    reward -= 0.01f;

    bool done = (state.currentState != State::InProgress);
    return { reward, done };
}

py::array_t<bool> PandemicEnv::GetValidMask()
{
    
    ActionList legalMoves;
    state.GetPossibleActions(legalMoves);

    // Create a numpy array of size 324 filled with False
    auto result = py::array_t<bool>(324);
    py::buffer_info buf = result.request();
    bool* ptr = static_cast<bool*>(buf.ptr);
    std::fill(ptr, ptr + 324, false);

    // Fill in the True values
    for (int i = 0; i < legalMoves.count; i++) {
        int idx = ActionDecoder::GetIndexFromAction(legalMoves.Get(i));
        if (idx >= 0 && idx < 324) ptr[idx] = true;
    }

    return result;
}

py::array_t<float> PandemicEnv::GetTensor()
{
    auto result = py::array_t<float>(783);
    py::buffer_info buf = result.request();
    float* ptr = static_cast<float*>(buf.ptr);

    int idx = 0;

    // 1. City Data (48 cities * 5 features = 240 floats)
    // Normalized cube counts and station presence
    for (int i = 0; i < NUMBER_OF_CITIES; i++) {
        ptr[idx++] = state.cityState.GetDiseaseCount(i, BLACK) / 3.0f;
        ptr[idx++] = state.cityState.GetDiseaseCount(i, BLUE) / 3.0f;
        ptr[idx++] = state.cityState.GetDiseaseCount(i, RED) / 3.0f;
        ptr[idx++] = state.cityState.GetDiseaseCount(i, YELLOW) / 3.0f;
        ptr[idx++] = state.cityState.HasStation(i) ? 1.0f : 0.0f;
    }

    // 2. Player Locations (4 players * 48 cities = 192 floats)
    // One-hot encoding: 1.0 if player is at city, 0.0 otherwise.
    for (int p = 0; p < NUMBER_OF_MAX_PLAYERS; p++) {
        int playerLoc = -1;

        // Only get location if this player actually exists in the current game
        if (p < state.players.count) {
            playerLoc = state.players.GetLocation(p);
        }

        for (int c = 0; c < NUMBER_OF_CITIES; c++) {
            ptr[idx++] = (playerLoc == c) ? 1.0f : 0.0f;
        }
    }

    // 3. Player Hands (Total Cards * 4 Players = 212 floats)
    // One-hot: Does Player P hold Card C?
    int total_cards = NUMBER_OF_CITY_CARDS + NUMBER_OF_EVENT_CARDS;

    for (int cardId = 0; cardId < total_cards; cardId++) {
        for (int p = 0; p < NUMBER_OF_MAX_PLAYERS; p++) {
            bool hasCard = false;

            if (p < state.players.count) {
                hasCard = state.players.HasCard(p, cardId);
            }
            ptr[idx++] = hasCard ? 1.0f : 0.0f;
        }
    }

    // 4. Player Roles (4 players * 7 roles = 28 floats)
    // One-hot encoding of the Role Enum
    for (int p = 0; p < NUMBER_OF_MAX_PLAYERS; p++) {
        int role = -1;
        if (p < state.players.count) {
            role = (int)state.players.GetRole(p);
        }

        for (int r = 0; r < NUMBER_OF_ROLE_CARDS; r++) {
            ptr[idx++] = (role == r) ? 1.0f : 0.0f;
        }
    }

    // 5. Global State (10 floats)

    // a) Infection Rate (Normalized 0.0 to 1.0)
    // Max index is 6 (rates: 2,2,2,3,3,4,4)
    ptr[idx++] = state.gameFlags.GetInfectionRateIndex() / 6.0f;

    // b) Outbreaks (Normalized 0.0 to 1.0)
    // Max outbreaks is 8
    ptr[idx++] = state.gameFlags.GetOutbreaks() / 8.0f;

    // c) Cured Status (4 floats)
    ptr[idx++] = state.gameFlags.IsCured(BLACK) ? 1.0f : 0.0f;
    ptr[idx++] = state.gameFlags.IsCured(BLUE) ? 1.0f : 0.0f;
    ptr[idx++] = state.gameFlags.IsCured(RED) ? 1.0f : 0.0f;
    ptr[idx++] = state.gameFlags.IsCured(YELLOW) ? 1.0f : 0.0f;

    // d) Eradicated Status (4 floats)
    ptr[idx++] = state.gameFlags.IsEradicated(BLACK) ? 1.0f : 0.0f;
    ptr[idx++] = state.gameFlags.IsEradicated(BLUE) ? 1.0f : 0.0f;
    ptr[idx++] = state.gameFlags.IsEradicated(RED) ? 1.0f : 0.0f;
    ptr[idx++] = state.gameFlags.IsEradicated(YELLOW) ? 1.0f : 0.0f;

    // 6. Infection Discard Pile (48 floats)
    // Bitmap: 1.0 if the city card is in the infection discard pile (danger of intensifying!)

    // First, map the discard pile to a temporary bool array for O(1) lookup
    bool isInInfectionDiscard[NUMBER_OF_CITIES] = { false };

    // Iterate the deck range
    for (uint8_t cardId : state.decks.infection_deck.GetDiscardPile()) {
        if (cardId < NUMBER_OF_CITIES) {
            isInInfectionDiscard[cardId] = true;
        }
    }

    for (int c = 0; c < NUMBER_OF_CITIES; c++) {
        ptr[idx++] = isInInfectionDiscard[c] ? 1.0f : 0.0f;
    }

    // 7. Player Discard Pile (53 floats)
    // We need to know exactly which cards are gone.

    // A. Track specific unique cards (Cities + Events)
    int total_unique_cards = NUMBER_OF_CITY_CARDS + NUMBER_OF_EVENT_CARDS;
    std::vector<bool> isDiscarded(total_unique_cards, false);

    // Iterate the Player Discard Pile
    for (uint8_t cardId : state.decks.player_deck.GetDiscardPile()) {
        if (CardRegistry::IsEpidemic(cardId)) {
        }
        else if (cardId < total_unique_cards) {
            isDiscarded[cardId] = true;
        }
    }

    // Append Bitmap to Tensor
    for (int i = 0; i < total_unique_cards; i++) {
        ptr[idx++] = isDiscarded[i] ? 1.0f : 0.0f;
    }

    return result;
}

float PandemicEnv::CalculateHeuristicScore()
{
    float score = 0.0f;

    // 1. Curing diseases is the Goal (Big points!)
    if (state.gameFlags.IsCured(BLACK)) score += 10.0f;
    if (state.gameFlags.IsCured(BLUE))  score += 10.0f;
    if (state.gameFlags.IsCured(RED))   score += 10.0f;
    if (state.gameFlags.IsCured(YELLOW)) score += 10.0f;

    // 2. Eradicating is even better (Bonus)
    if (state.gameFlags.IsEradicated(BLACK)) score += 2.0f;
    // ... repeat for others ...

    // 3. Treating cubes is good (Small points)
    // We want to penalize having cubes on the board.
    int totalCubes = 0;
    for (int i = 0; i < ColorType::COUNT; i++) {
        totalCubes += state.cityState.GetTotalCubeCount((ColorType)i);
    }
    score -= (totalCubes * 0.1f); // -0.1 per cube on board

    // 4. Outbreaks are bad!
    score -= (state.gameFlags.GetOutbreaks() * 2.0f);

    return score;
}

PYBIND11_MODULE(pandemic_cpp, m) {
    m.doc() = "Optimized C++ Pandemic Engine for RL";

    py::class_<PandemicEnv>(m, "PandemicEnv")
        .def(py::init<int>(), "seed"_a = 42)
        .def("reset", &PandemicEnv::Reset)
        .def("step", &PandemicEnv::Step)
        .def("get_tensor", &PandemicEnv::GetTensor)
        .def("get_valid_mask", &PandemicEnv::GetValidMask);
}
