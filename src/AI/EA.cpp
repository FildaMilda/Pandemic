#include "EA.h"
#include <omp.h>

FitnessResult EvaluateFitness(const Weights& w, int gamesToPlay)
{
    int wins = 0;
    int totalActions = 0;
    int totalCures = 0;
    GameResult result;

    for (int i = 0; i < gamesToPlay; ++i) {
        result = PlayGameMCTS(
            Difficulty::INTRO,
            4,
            i,
            w
        );
        totalActions += result.actionCount;

        if (result.finalState == State::AllCured) wins++;
    }

    totalCures = result.state.gameFlags.IsCured(ColorType::BLACK) + result.state.gameFlags.IsCured(ColorType::BLUE) + result.state.gameFlags.IsCured(ColorType::RED) + result.state.gameFlags.IsCured(ColorType::YELLOW);

    return FitnessResult{ (float)wins / gamesToPlay, totalActions, totalCures};
}

void EvolveWeights() {
    const int POPULATION_SIZE = 20;
    const int GENERATIONS = 50;
    const int TOP_SURVIVORS = 5;
    std::mt19937 rng(1337);

    std::vector<Weights> population(POPULATION_SIZE);
    for (auto& individual : population) {
        individual.Randomize(rng);
    }
    std::vector<FitnessResult> fitness(POPULATION_SIZE);

    for (int gen = 0; gen < GENERATIONS; ++gen) {
        std::cout << "--- Generation " << gen << " ---\n";

        // 1. Evaluate each individual
        for (int i = 0; i < POPULATION_SIZE; ++i) {
            fitness[i] = EvaluateFitness(population[i], 200);
            std::cout << "  Individual " << i << " Win Rate: " << fitness[i].winRate << "\n";
        }

        // 2. Sort by fitness while keeping weights and scores linked
        // We create an index array [0, 1, 2, ..., 19]
        std::vector<int> indices(POPULATION_SIZE);
        std::iota(indices.begin(), indices.end(), 0);

        // SORTING LOGIC: Win rate first, then total actions
        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            if (fitness[a].winRate != fitness[b].winRate) {
                return fitness[a].winRate > fitness[b].winRate;
            }
            return fitness[a].actionCount > fitness[b].actionCount;
            });

        // Reorder
        std::vector<Weights> next_gen;
        next_gen.reserve(POPULATION_SIZE);

        // Keep the elites
        for (int i = 0; i < TOP_SURVIVORS; ++i) {
            next_gen.push_back(population[indices[i]]);
        }

        // Fill the rest with mutated elites
        for (int i = TOP_SURVIVORS; i < POPULATION_SIZE; ++i) {
            Weights child = next_gen[i % TOP_SURVIVORS];
            child.Mutate(0.05f, rng);
            next_gen.push_back(child);
        }

        population = std::move(next_gen);

        auto& best = fitness[indices[0]];
        std::cout << "Best: WinRate " << best.winRate << " | Actions: " << best.actionCount << "\n";
        population[indices[0]].Print();
    }
}

void EvolveWeightsParallel() {
    const int POPULATION_SIZE = 20;
    const int GENERATIONS = 50;
    const int TOP_SURVIVORS = 5;
    std::mt19937 rng(42);

    std::vector<Weights> population(POPULATION_SIZE);
    for (auto& individual : population) {
        individual.Randomize(rng);
    }

    std::vector<FitnessResult> fitnessResults(POPULATION_SIZE);
    std::vector<int> indices(POPULATION_SIZE);
    std::iota(indices.begin(), indices.end(), 0);

    for (int gen = 0; gen < GENERATIONS; ++gen) {
        std::cout << "--- Generation " << gen << " ---\n";

        // MULTI-CORE EVALUATION
        // std::execution::par tells the compiler to run this in parallel
        std::for_each(std::execution::par, indices.begin(), indices.end(), [&](int i) {
            fitnessResults[i] = EvaluateFitness(population[i], 200);
            });

        // Sort indices based on fitness (Win Rate first, then Actions)
        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            // 1. Check Win Rate
            if (fitnessResults[a].winRate != fitnessResults[b].winRate) {
                return fitnessResults[a].winRate > fitnessResults[b].winRate;
            }

            // 2. Win Rates are tied, check Total Cures
            if (fitnessResults[a].cureCount != fitnessResults[b].cureCount) {
                return fitnessResults[a].cureCount > fitnessResults[b].cureCount;
            }

            // 3. Cures are tied, check Action Count
            // Returning 'true' means 'a' comes before 'b'.
            // If you want the one with FEWER actions to be "better", use <
            return fitnessResults[a].actionCount > fitnessResults[b].actionCount;
            });

        // Prepare the next generation
        std::vector<Weights> next_gen;
        next_gen.reserve(POPULATION_SIZE);

        // 1. Elitism: Keep the best
        for (int i = 0; i < TOP_SURVIVORS; ++i) {
            next_gen.push_back(population[indices[i]]);
        }

        // 2. Repopulate: Mutate the survivors
        for (int i = TOP_SURVIVORS; i < POPULATION_SIZE; ++i) {
            Weights child = next_gen[i % TOP_SURVIVORS];
            child.Mutate(0.05f, rng);
            next_gen.push_back(child);
        }

        population = std::move(next_gen);

        // Logging the current best of this generation
        auto& bestResult = fitnessResults[indices[0]];
        std::cout << "Gen " << gen << " Best WR: " << bestResult.winRate
            << " | Actions: " << bestResult.actionCount << "\n";

        // Print the actual weight values of the current champion
        std::cout << "Best Weights: ";
        population[0].Print();
        std::cout << "--------------------" << std::endl;
    }
}