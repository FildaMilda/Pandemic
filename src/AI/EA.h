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

struct FitnessResult {
	float winRate;
	int actionCount;
};

FitnessResult EvaluateFitness(const Weights& w, int gamesToPlay);
void EvolveWeights();
void EvolveWeightsParallel();

#endif