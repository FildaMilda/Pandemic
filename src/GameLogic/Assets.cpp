#include "Assets.h"

bool PlayGameMCTS(Difficulty diff, uint8_t player_count, int seed)
{
    std::random_device rd;
    std::mt19937 realRng(seed);

    CardRegistry cards;
    cards.Initialize();

    GameState state;
    state.Setup(diff, player_count, &realRng);

    int action_count = 0;
    while (state.currentState == State::InProgress) {
        Action bestMove = MCTS::GetBestMove(state, 10000);
        state.Execute(bestMove);
        action_count++;
    }

	return state.currentState == State::AllCured;
}
