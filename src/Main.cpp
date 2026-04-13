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

    int game_count = 50;
    int cure_counter = 0;
    int win_counter = 0;

    for (int seed = 0; seed < game_count; seed++) {
        std::mt19937 realRng(12312 + seed);

        GameState state;
        state.Setup(Difficulty::INTRO, 4, &realRng);

        MacroMCTS brain;

        std::cout << std::format("Starting seed: {}", seed);
        while (state.currentState == State::InProgress) {
            uint8_t currentPlayer = state.gameFlags.GetActivePlayer();
            state.players.Print();

            std::cout << "\nPlayer " << (int)currentPlayer
                << " (" << state.players.GetRole(currentPlayer) << "):" << std::endl;

            Turn bestMacro = brain.Search(state, 10000);
            bestMacro.Print();

            std::cout << std::endl;

            state.Execute(bestMacro);

            std::cout << "Cured: " << (int)state.gameFlags.GetCuredCount()
                << " | Outbreaks: " << (int)state.gameFlags.GetOutbreaks()
                << " | Cubes: " << state.cityState.global_cubes[0] + state.cityState.global_cubes[1] + state.cityState.global_cubes[2] + state.cityState.global_cubes[3] << "\n";
        }
        std::cout << std::format("Final state: {}\n", (int)state.currentState);
        if (state.currentState == State::AllCured) win_counter++;
        cure_counter += state.gameFlags.GetCuredCount();
    }

    std::cout << std::format("cures: {}/{}, wins: {}/{}\n", cure_counter, game_count * 4, win_counter, game_count);
}