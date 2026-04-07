#include "PythonInterface.h"

using namespace pybind11::literals;

PandemicEnv PandemicEnv::Clone()
{
    PandemicEnv copy = *this;
    return copy;
}

py::array_t<float> PandemicEnv::Reset()
{
	state.Setup(Difficulty::INTRO, 4, &rng);
	return GetTensor();
}

std::pair<float, bool> PandemicEnv::Step(int actionIndex)
{
    Action action = ActionDecoder::GetActionFromIndex(actionIndex, state);

    state.Execute(action);
    //action.Print();

    bool done = state.currentState != State::InProgress;
    float reward = 0.0f;

    if (done) {
        reward = state.currentState == State::AllCured ? 1.0f : -1.0f;
    }
    else {
        reward = GetScore();
    }

    return { reward, done };
}

py::array_t<bool> PandemicEnv::GetValidMask()
{
    // Use the actual total count defined in your ActionRanges
    int size = ActionRanges::COUNT;

    ActionList legalMoves;
    state.GetPossibleActions(legalMoves);

    // Explicitly create the array with the correct shape
    auto result = py::array_t<bool>({ size });
    auto buf = result.request();
    bool* ptr = static_cast<bool*>(buf.ptr);

    // Initialize everything to false (0)
    std::fill(ptr, ptr + size, false);

    bool atLeastOneValid = false;

    for (int i = 0; i < legalMoves.count; i++) {
        int idx = ActionDecoder::GetIndexFromAction(legalMoves.Get(i));

        // BUG FIX: Added logging if the decoder returns an out-of-bounds index
        if (idx >= 0 && idx < size) {
            ptr[idx] = true;
            atLeastOneValid = true;
        }
        else {
            // This is how you catch the bug where an action is "legal" 
            // but your Decoder doesn't know how to map it yet!
            py::print("WARNING: Legal action has no valid index! Index:", idx);
        }
    }

    // CRITICAL: If no moves are valid, the AI will crash Python's np.random.choice.
    // We should never have 0 legal moves in Pandemic unless the game is over.
    if (!atLeastOneValid && state.currentState == State::InProgress) {
        py::print("CRITICAL: No legal moves found for an active game state!");
    }

    return result;
}

py::array_t<float> PandemicEnv::GetTensor()
{
    std::vector<float> obs = state.ToTensor();
    return py::array_t<float>(obs.size(), obs.data());
}

py::array_t<float> PandemicEnv::RunMCTS(int iterations, py::object model)
{
    std::vector<float> pi = MCTS_NN::RunSearch(this->state, iterations, model);
    return py::array_t<float>(pi.size(), pi.data());
}

py::dict PandemicEnv::GetGameInfo()
{
    py::dict info;

    // 1. Game Status
    std::string status = "InProgress";
    if (state.currentState == State::AllCured) status = "Win_AllCured";
    else if (state.currentState == State::OutbreakMarkerMaxed) status = "Loss_Outbreaks";
    else if (state.currentState == State::NotEnoughPlayerCards) status = "Loss_DeckEmpty";
    else if (state.currentState == State::NoMoreDiseaseCubes) status = "Loss_CubesEmpty";

    info["status"] = status;

    // 2. Progression Metrics
    info["outbreaks"] = (int)state.gameFlags.outbreak_counter;
    info["infection_rate_idx"] = (int)state.gameFlags.infection_rate_idx;

    // 3. Diseases Cured
    py::list cured_list;
    if (state.gameFlags.IsCured(ColorType::BLUE)) cured_list.append("Blue");
    if (state.gameFlags.IsCured(ColorType::YELLOW)) cured_list.append("Yellow");
    if (state.gameFlags.IsCured(ColorType::BLACK)) cured_list.append("Black");
    if (state.gameFlags.IsCured(ColorType::RED)) cured_list.append("Red");
    info["cured_diseases"] = cured_list;
    info["cured_count"] = state.gameFlags.GetCuredCount();

    // 4. Roles in Play
    py::list roles_list;
    for (int i = 0; i < state.players.count; ++i) {
        uint8_t role_id = state.players.roles[i];
        roles_list.append(role_id);
    }
    info["player_roles"] = roles_list;

    return info;
}

float PandemicEnv::GetScore()
{
    return CalculateHeuristicScore(state, Weights());
}

PYBIND11_MODULE(pandemic_cpp, m) {
    m.doc() = "Optimized C++ Pandemic Engine for RL";

    static bool initialized = false;
    if (!initialized) {
        CardRegistry cards;
        cards.Initialize();

        MapData::PrecomputeDistancesAndPaths();

        initialized = true;
    }

    py::class_<PandemicEnv>(m, "PandemicEnv")
        .def(py::init<int>(), "seed"_a = 42)
        .def("reset", &PandemicEnv::Reset)
        .def("step", &PandemicEnv::Step)
        .def("get_tensor", &PandemicEnv::GetTensor)
        .def("get_valid_mask", &PandemicEnv::GetValidMask)
        .def("run_mcts", &PandemicEnv::RunMCTS)
        .def("get_info", &PandemicEnv::GetGameInfo);
}
