#include "Assets.h"
#include <format>

GameResult PlayGameMCTS(Difficulty diff, uint8_t player_count, int seed, const Weights& weights, int mcts_iterations, bool print_info)
{
    std::mt19937 realRng(seed);

    //CardRegistry cards;
    //cards.Initialize();

    static CardRegistry cards;
    static std::once_flag flag;
    std::call_once(flag, []() { cards.Initialize(); });

    MapData::PrecomputeDistancesAndPaths();

    GameState state;
    state.Setup(diff, player_count, &realRng);

    int action_count = 0;
    while (state.currentState == State::InProgress) {
        Action bestMove = MCTS::GetBestMove(state, mcts_iterations, weights, print_info);
        state.Execute(bestMove);
        action_count++;
    }

    return GameResult{ state, state.currentState, action_count };
}
