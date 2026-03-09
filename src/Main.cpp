#include <iostream>
#include <random>
#include "Game.h"
#include "Cards.h"
#include "MCTS.h"
#include "EA.h"

#include <fstream>
#include <iostream>

enum MainType {
    DebugMultipleGames,
    DebugOneGame,
    Evolve
};

int main()
{
    MainType type = MainType::Evolve;

    if (type == MainType::Evolve) {
        EvolveWeightsParallel();
    }


    if (type == MainType::DebugOneGame) {
        CardRegistry cards;
        cards.Initialize();

        MapData::PrecomputeDistances();

        int seed = 205;
        std::mt19937 realRng(seed);

        GameState state;
        state.Setup(Difficulty::INTRO, 4, &realRng);

        int action_count = 0;
        while (state.currentState == State::InProgress) {
            state.cityState.Print();

            Action bestMove = MCTS::GetBestMove(state, 1000, Weights());
            state.Execute(bestMove);
            action_count++;
        }

        std::cout << std::format("Final state: {}", (int)state.currentState);
    }

    if (type == MainType::DebugMultipleGames) {
        CardRegistry cards;
        cards.Initialize();

        MapData::PrecomputeDistances();

        int number_of_games = 100;

        int total_actions = 0;
        int total_cures = 0;
        int win_count = 0;
        int outbreak_loss_count = 0;
        int cubes_loss_count = 0;
        int deck_loss_count = 0;

        for (int i = 0; i < number_of_games; i++) {
            std::cout << std::format("== Game number: {} ==\n", i);

            int seed = i + 123;
            std::mt19937 realRng(seed);

            GameState state;
            state.Setup(Difficulty::INTRO, 4, &realRng);

            int action_count = 0;
            while (state.currentState == State::InProgress) {
                Action bestMove = MCTS::GetBestMove(state, 1000, Weights());
                state.Execute(bestMove);
                action_count++;
            }

            int diseases_cured = state.gameFlags.IsCured(ColorType::BLACK) + state.gameFlags.IsCured(ColorType::BLUE) + state.gameFlags.IsCured(ColorType::RED) + state.gameFlags.IsCured(ColorType::YELLOW);
            std::cout << std::format("Game results (seed={}):\nFinal state: {} Cured: {}/4 Actions: {}\n\n", seed, (int)state.currentState, diseases_cured, action_count);

            total_actions += action_count;
            total_cures += diseases_cured;
            switch (state.currentState) {
            case AllCured:
                win_count += 1;
                break;
            case OutbreakMarkerMaxed:
                outbreak_loss_count += 1;
                break;
            case NoMoreDiseaseCubes:
                cubes_loss_count += 1;
                break;
            case NotEnoughPlayerCards:
                deck_loss_count += 1;
                break;
            }
        }

        std::cout << std::format("=== RESULTS ===\n");
        std::cout << std::format("Total actions: {}\n", total_actions);
        std::cout << std::format("Total cures: {}/{} ({}%)\n", total_cures, number_of_games * 4, (float)total_cures / ((float)number_of_games * 4) * 100);
        std::cout << std::format("Wins: {}/{} ({}%)\n", win_count, number_of_games, (float)win_count / (float)number_of_games * 100);
        std::cout << std::format("Outbreak: {}/{} ({}%)\n", outbreak_loss_count, number_of_games, (float)outbreak_loss_count / (float)number_of_games * 100);
        std::cout << std::format("Cubes: {}/{} ({}%)\n", cubes_loss_count, number_of_games, (float)cubes_loss_count / (float)number_of_games * 100);
        std::cout << std::format("Deck: {}/{} ({}%)\n", deck_loss_count, number_of_games, (float)deck_loss_count / (float)number_of_games * 100);
    }
}