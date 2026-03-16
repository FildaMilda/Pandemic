#ifndef EA_H
#define EA_H

#include "Globals.h"
#include "Assets.h"
#include <iostream>
#include <algorithm>
#include <random>
#include <numeric>
#include <execution>
#include <cstdint>

Weights EvolveWeightsForSeed(Difficulty diff, uint8_t player_count, int target_seed, int generations, int pop_size);
float EvaluateFitness(const GameResult& result);

#endif