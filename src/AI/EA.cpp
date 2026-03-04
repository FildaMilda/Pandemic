#include "EA.h"

float EvaluateFitness(const Weights& w, int gamesToPlay)
{
    int wins = 0;
    int totalTurns = 0;

    for (int i = 0; i < gamesToPlay; ++i) {
        bool won = PlayGameMCTS(
            Difficulty::INTRO,
            4,
            i
        );

        if (won) wins++;
    }

    // Fitness is primarily Win Rate, secondary is how long they survived
    return (float)wins / gamesToPlay;
}

void EvolveWeights() {
    const int POPULATION_SIZE = 20;
    const int GENERATIONS = 50;
    const int TOP_SURVIVORS = 5;
    std::mt19937 rng(1337);

    std::vector<Weights> population(POPULATION_SIZE);
    std::vector<float> fitness(POPULATION_SIZE);

    for (int gen = 0; gen < GENERATIONS; ++gen) {
        std::cout << "--- Generation " << gen << " ---\n";

        // 1. Evaluate each individual
        for (int i = 0; i < POPULATION_SIZE; ++i) {
            fitness[i] = EvaluateFitness(population[i], 200);
            std::cout << "  Individual " << i << " Win Rate: " << fitness[i] << "\n";
        }

        // 2. Sort by fitness while keeping weights and scores linked
        // We create an index array [0, 1, 2, ..., 19]
        std::vector<int> indices(POPULATION_SIZE);
        std::iota(indices.begin(), indices.end(), 0);

        // Sort indices based on the values in the fitness vector (descending)
        std::sort(indices.begin(), indices.end(), [&](int a, int b) {
            return fitness[a] > fitness[b];
            });

        // Reorder the population based on the sorted indices
        std::vector<Weights> next_population(POPULATION_SIZE);
        for (int i = 0; i < POPULATION_SIZE; ++i) {
            next_population[i] = population[indices[i]];
        }

        // Update current population with the sorted one
        population = std::move(next_population);
        // Also update the best fitness for logging
        float best_fitness = fitness[indices[0]];

        // 3. Reproduce and Mutate
        // The top 5 (0-4) are already at the front of 'population'.
        // We replace individuals 5 through 19 with mutated versions of 0-4.
        for (int i = TOP_SURVIVORS; i < POPULATION_SIZE; ++i) {
            // Pick a parent from the top 5 (modulo ensures we cycle through them)
            population[i] = population[i % TOP_SURVIVORS];

            // Apply mutation
            population[i].Mutate(0.05f, rng);
        }

        std::cout << "Gen " << gen << " Best Win Rate: " << best_fitness << std::endl;

        // Optional: Save the best weights to a file or print them
        if (gen % 5 == 0) {
            population[0].Print();
        }
    }
}
