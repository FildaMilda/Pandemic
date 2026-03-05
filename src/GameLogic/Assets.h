#ifndef ASSETS_H
#define ASSETS_H

#include "Globals.h"
#include "Game.h"
#include "MCTS.h"

struct GameResult {
	State finalState;
	int actionCount;
};

GameResult PlayGameMCTS(Difficulty diff, uint8_t player_count, int seed, const Weights& weights);

#endif
