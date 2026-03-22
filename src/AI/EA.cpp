#include "EA.h"
#include <omp.h>

float EvaluateFitness(const GameResult& result) {
    // If we won, massive score. Subtracting actionCount rewards faster, more efficient wins.
    if (result.finalState == State::AllCured) {
        return 10000.0f - result.actionCount;
    }

    float fitness = 0.0f;

    // Use the state to determine how close it was to winning.
    // (Assuming these getters based on your previous state structures)
    fitness += result.state.gameFlags.GetCuredCount() * 1000.0f;
    fitness -= result.state.gameFlags.GetOutbreaks() * 100.0f;

    // Slight penalty for high cube presence at the end of the game
    int total_cubes = 0;
    for (int c = 0; c < 4; ++c) {
        total_cubes += result.state.cityState.GetTotalCubeCount((ColorType)c);
    }
    fitness -= total_cubes * 5.0f;

    // Reward it slightly for surviving longer (more actions) before a loss
    fitness += result.actionCount * 2.0f;

    return fitness;
}

Weights EvolveWeightsForSeed(Difficulty diff, uint8_t player_count, int target_seed, int generations, int pop_size, int mcts_iterations) {
    std::mt19937 rng(std::random_device{}());

    struct Individual {
        Weights weights;
        float fitness;
        GameResult lastResult;
    };

    std::vector<Individual> population(pop_size);

    // Initialize population randomly
    for (int i = 1; i < pop_size; ++i) {
        population[i].weights.Mutate(1.0f, rng);
    }

    std::cout << "Starting Evolution for Seed: " << target_seed << "\n";
    std::cout << "=========================================\n";

    for (int gen = 0; gen < generations; ++gen) {
        // Evaluate fitness for the entire population
        for (int i = 0; i < pop_size; ++i) {
            // Only re-evaluate if fitness hasn't been calculated (e.g., for new mutants)
            if (gen == 0 || i > 0) {
                population[i].lastResult = PlayGameMCTS(diff, player_count, target_seed, population[i].weights, mcts_iterations, false);
                population[i].fitness = EvaluateFitness(population[i].lastResult);
            }
        }

        // Sort by fitness (Descending - highest fitness first)
        std::sort(population.begin(), population.end(), [](const Individual& a, const Individual& b) {
            return a.fitness > b.fitness;
            });

        Individual best = population[0];

        std::cout << "Gen " << gen << " | Best Fitness: " << best.fitness
            << " | Cures: " << (int)best.lastResult.state.gameFlags.GetCuredCount()
            << " | End State: " << (int)best.lastResult.finalState << "\n";
        best.weights.Print();

        // Early Exit if we achieved a win
        if (best.lastResult.finalState == State::AllCured) {
            std::cout << "\n>>> WIN ACHIEVED IN GENERATION " << gen << " <<<\n";
            std::cout << "Winning Weights:\n";
            best.weights.Print();
            return best.weights;
        }

        // Create the next generation (Elitism: Keep the best individual at index 0)
        std::vector<Individual> next_generation(pop_size);
        next_generation[0] = best; // Elite survives unchanged

        // Fill the rest with mutated versions of the top performers
        int tournament_size = std::max(2, pop_size / 4);
        for (int i = 1; i < pop_size; ++i) {
            // Pick a random top performer to act as the parent
            std::uniform_int_distribution<int> parent_dist(0, tournament_size - 1);
            int parent_idx = parent_dist(rng);

            next_generation[i].weights = population[parent_idx].weights;

            // Mutate. As generations go on, we can optionally decay the mutation rate
            // to fine-tune the weights rather than making wild swings.
            float current_mutation_rate = (1.0f - ((float)gen / generations));
            next_generation[i].weights.Mutate(current_mutation_rate, rng);

            // Ensure no negative weights if your heuristic requires them to be positive
            next_generation[i].weights.cure_weight = std::max(0.0f, next_generation[i].weights.cure_weight);
            next_generation[i].weights.cube_pressure = std::max(0.0f, next_generation[i].weights.cube_pressure);
            // ... apply to other critical weights as necessary
        }

        population = std::move(next_generation);
    }

    std::cout << "\nEvolution finished without a win. Returning best found weights.\n";
    population[0].weights.Print();
    return population[0].weights;
}