#ifndef ASSETS_H
#define ASSETS_H

#include "Globals.h"
#include "Game.h"
#include "MCTS.h"

#include <mutex>

struct GameResult {
	GameState state;
	State finalState;
	int actionCount;
};

GameResult PlayGameMCTS(Difficulty diff, uint8_t player_count, int seed, const Weights& weights, int mcts_iterations, bool print_info);

#endif
