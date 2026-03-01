#include <iostream>
#include <random>
#include "Game.h"
#include "Cards.h"
#include "MCTS.h"

#include <fstream>
#include <iostream>

int main()
{
    std::random_device rd;
    std::mt19937 realRng(71);

    CardRegistry cards;
    cards.Initialize();
     
    GameState state; 
    state.Setup(&realRng);

    /*
    ActionList legalMoves;

    while (state.currentState == State::InProgress) {
        state.decks.player_deck.Print();
        state.decks.infection_deck.Print();
        state.players.Print();
        state.cityState.Print();

        state.GetPossibleActions(legalMoves);
        legalMoves.Print();

        std::uniform_int_distribution<int> dist(0, legalMoves.count-1);
        int random_number = dist(realRng);

        Action action = legalMoves.Get(random_number);
        std::cout << "Action taken (idx: " << random_number << "): ";
        action.Print();
        state.Execute(action);
    }

    std::cout << "Game Ended, reason: " << (int)state.currentState;
    */

    /*
    for (int i = 0; i < 10000; i++) {
        std::mt19937 realRng(i);

        CardRegistry cards;
        cards.Initialize();

        GameState state;
        state.Setup(&realRng);
        ActionList legalMoves;

        while (state.currentState == State::InProgress) {
            state.GetPossibleActions(legalMoves);

            std::uniform_int_distribution<int> dist(0, legalMoves.count - 1);
            int random_number = dist(realRng);

            Action action = legalMoves.Get(random_number);
            state.Execute(action);
        }

        std::cout << "Game Ended (" << i << "), reason: " << (int)state.currentState << "\n";
    }
    */

    
    int action_count = 0;
    while (state.currentState == State::InProgress) {
        // 2. Think! (Run 1000 simulations)
        // The higher the number, the smarter (and slower) it gets.
        Action bestMove = MCTS::GetBestMove(state, 10000);

        // 3. Execute
        std::cout << "[" << action_count << "]" << " MCTS Chose: ";
        bestMove.Print();
        state.Execute(bestMove);
        action_count++;
    }

    int i = 0;
    std::cout << "Game Ended (" << i << "), reason: " << (int)state.currentState << "\n";
    
}