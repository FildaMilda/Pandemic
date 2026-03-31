#include <iostream>
#include <random>
#include "Game.h"
#include "Cards.h"
#include "MCTS.h"
#include "EA.h"
#include "ActionDecoder.h"

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
    MainType type = MainType::Test;

    if (type == MainType::DebugOneGame) {
        CardRegistry cards;
        cards.Initialize();

        MapData::PrecomputeDistances();

        int seed = 3768;
        std::mt19937 realRng(seed);

        GameState state;
        state.Setup(Difficulty::INTRO, 4, &realRng);

        ActionList actions;

        int action_count = 0;
        
        while (state.currentState == State::InProgress) {
            //state.cityState.Print();

            Action move = MCTS::GetBestMove(state, 1000, Weights(), true);
            state.Execute(move);
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
                Action bestMove = MCTS::GetBestMove(state, 1000, Weights(), false);
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

    if (type == ID_ACTION_TEST) {
        int action_count = 0;
        int index;
        Action action;

        CardRegistry cards;
        cards.Initialize();

        MapData::PrecomputeDistances();

        int seed = 205;
        std::mt19937 realRng(seed);

        GameState state;
        state.Setup(Difficulty::INTRO, 4, &realRng);

        std::cout << state.ToTensor().size() << "\n";

        int gov_1_id = ActionRanges::GOVERMENT_GRANT_START + 1;

        while (true) {
            action = ActionDecoder::GetActionFromIndex(action_count, state);
            if (action.raw_data == std::numeric_limits<uint32_t>::max()) break;

            index = ActionDecoder::GetIndexFromAction(action);

            if (index != action_count) 
                std::cout << std::format("From {}, we got action {} and from that, we got {}\n", action_count, action.raw_data, index);

            action_count++;
        }

        std::cout << ActionRanges::COUNT << "\n";
        std::cout << action_count;
    }

    if (type == MCTS_PLAY) {
        auto res = PlayGameMCTS(Difficulty::INTRO, 4, 123, Weights(), 1000000, true);
        std::cout << "Final state: " << (int)res.finalState << " Action count: " << res.actionCount << " Cure count: " << (int)res.state.gameFlags.GetCuredCount() << "\n";
    }

    if (type == EAOneSeed) {
        EvolveWeightsForSeed(Difficulty::INTRO, 4, 123, 10000, 50, 1000);
    }

    if (type == Test) {

        CardRegistry cards;
        cards.Initialize();

        MapData::PrecomputeDistances();

        int longestDist = 0;
        int a;
        int b;
        for (int city_a = 0; city_a < NUMBER_OF_CITIES; city_a++) {
            for (int city_b = 0; city_b < NUMBER_OF_CITIES; city_b++) {
                int dist = MapData::GetDistance(city_a, city_b);
                if (dist > longestDist) {
                    longestDist = dist;
                    a = city_a;
                    b = city_b;
                }
            }
        }
        std::cout << "Longest drive distance between two cities is: " << longestDist << " Between " << CardRegistry::GetName(a) << " and " << CardRegistry::GetName(b) << "\n";

    }
}