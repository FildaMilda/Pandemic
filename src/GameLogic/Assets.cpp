#include "Assets.h"
#include <format>

GameResult PlayGameMCTS(Difficulty diff, uint8_t player_count, int seed, const Weights& weights)
{
    std::mt19937 realRng(seed);

    //CardRegistry cards;
    //cards.Initialize();

    static CardRegistry cards;
    static std::once_flag flag;
    std::call_once(flag, []() { cards.Initialize(); });

    MapData::PrecomputeDistances();

    GameState state;
    state.Setup(diff, player_count, &realRng);

    int action_count = 0;
    while (state.currentState == State::InProgress) {
        Action bestMove = MCTS::GetBestMove(state, 100000, weights);
        state.Execute(bestMove);
        action_count++;
    }

    return GameResult{ state, state.currentState, action_count };
}
