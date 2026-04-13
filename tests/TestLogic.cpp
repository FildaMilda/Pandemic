#include "catch.hpp"
#include "Game.h"

// --- Helper Function ---
// Keeps our tests clean by setting up a reliable starting board
GameState CreateTreatScenario() {
    GameState state;
    std::mt19937 realRng(42);
    state.Setup(Difficulty::INTRO, 4, &realRng);

    // Washington (ID 1) has 1 Blue cube, New York (ID 2) has 2 Blue cubes
    state.cityState.AddDisease(static_cast<uint8_t>(City::Washington), ColorType::BLUE);
    state.cityState.AddDiseases(static_cast<uint8_t>(City::NewYork), ColorType::BLUE, 2);

    return state;
}

TEST_CASE("Macro Execution: Drive and Treat Sequence", "[Execute][Macro]") {
    // 1. ARRANGE
    GameState state = CreateTreatScenario();

    // 2. ACT
    // Create a Macro: Drive to Washington -> Drive to New York -> Treat Blue
    Turn macro;
    macro.Add(Action(DRIVE, (uint8_t)City::Washington, 0, 0));
    macro.Add(Action(DRIVE, (uint8_t)City::NewYork, 0, 0));
    macro.Add(Action(TREAT, (uint8_t)City::NewYork, 0, ColorType::BLUE));

    state.Execute(macro);

    // 3. ASSERT
    SECTION("Player location is updated correctly") {
        REQUIRE(state.players.GetLocation(0) == (uint8_t)City::NewYork);
    }

    SECTION("Action Points are consumed correctly") {
        // Macro had 3 actions. We started with 4. Should have 1 left.
        REQUIRE(state.gameFlags.GetActionsRemaining() == 1);
    }

    SECTION("Disease cubes are removed correctly") {
        // NY started with 2 cubes, we treated 1. Should have 1 left.
        REQUIRE(state.cityState.GetCubeCount((uint8_t)City::NewYork, ColorType::BLUE) == 1);

        // Washington should be untouched
        REQUIRE(state.cityState.GetCubeCount((uint8_t)City::Washington, ColorType::BLUE) == 1);
    }
}