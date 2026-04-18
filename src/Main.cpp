#include <iostream>
#include <random>
#include "Game.h"
#include "Cards.h"
#include "MCTS.h"
#include "EA.h"
#include "ActionDecoder.h"
#include "MacroMCTS.h"

#include <fstream>
#include <iostream>

enum MainType {
    DebugMultipleGames,
    DebugOneGame,
    Evolve,
    ID_ACTION_TEST,
    MCTS_PLAY,
    EAOneSeed,
    Test
};

int main()
{
    CardRegistry cards;
    cards.Initialize();

    MapData::PrecomputeDistancesAndPaths();

    int game_count = 100;
    int cure_counter = 0;
    int win_counter = 0;
    int outreak_loss_counter = 0;
    int cube_loss_counter = 0;
    int time_loss_counter = 0;

    for (int i = 0; i < game_count; i++) {
        int seed = 422134 + i;
        std::mt19937 realRng(seed);

        GameState state;
        state.Setup(Difficulty::INTRO, 4, &realRng);

        MacroMCTS brain;

        std::cout << std::format("Starting seed: {}", seed);
        while (state.currentState == State::InProgress) {
            uint8_t currentPlayer = state.gameFlags.GetActivePlayer();

            /*
            state.cityState.Print();
            state.players.Print();

            std::cout << "\nPlayer " << (int)currentPlayer
                << " (" << state.players.GetRole(currentPlayer) << "):" << std::endl;
            */

            Turn bestMacro = brain.Search(state, 10000);

            /*
            bestMacro.Print();
            std::cout << std::endl;
            */

            state.Execute(bestMacro);

            /*
            std::cout << "Cured: " << (int)state.gameFlags.GetCuredCount()
                << " | Outbreaks: " << (int)state.gameFlags.GetOutbreaks()
                << " | Cubes: " << state.cityState.global_cubes[0] + state.cityState.global_cubes[1] + state.cityState.global_cubes[2] + state.cityState.global_cubes[3] << "\n";
            */
        }

        std::cout << std::format("Final state: {}\n", (int)state.currentState);
        if (state.currentState == State::AllCured) win_counter++;
        if (state.currentState == State::OutbreakMarkerMaxed) outreak_loss_counter++;
        if (state.currentState == State::NoMoreDiseaseCubes) cube_loss_counter++;
        if (state.currentState == State::NotEnoughPlayerCards) time_loss_counter++;
        cure_counter += state.gameFlags.GetCuredCount();
    }

    std::cout << std::format("cures: {}/{}, wins: {}, outbreak: {}, cube: {}, time: {}\n", cure_counter, game_count * 4, win_counter, outreak_loss_counter, cube_loss_counter, time_loss_counter);
}