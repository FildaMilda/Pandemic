#include <iostream>
#include <random>
#include "Game.h"
#include "Cards.h"
#include "MCTS.h"
#include "EA.h"

#include <fstream>
#include <iostream>

int main()
{
    std::random_device rd;
    std::mt19937 realRng(42);

    CardRegistry cards;
    cards.Initialize();
     
    GameState player_state; 
    player_state.Setup(Difficulty::INTRO, 4, &realRng);
    
    GameState ai_state;
    ai_state.Setup(Difficulty::INTRO, 4, &realRng);

    ActionList player_actions;
    ActionList ai_actions;

    while (player_state.currentState == State::InProgress && ai_state.currentState == State::InProgress) {
        player_state.GetPossibleActions(player_actions);
        ai_state.GetFilteredActions(ai_actions);

        std::cout << "Player: " << player_actions.count << " AI: " << ai_actions.count << "\n";

        player_state.Execute(player_actions.Get(0));
        ai_state.Execute(ai_actions.Get(0));
    }
}