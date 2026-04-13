#define CATCH_CONFIG_RUNNER
#include "../extern/catch.hpp"
#include "Map.h"
#include "Deck.h"

int main(int argc, char* argv[]) {
    // --- ONE-TIME INITIALIZATION ---
    // This runs once when PandemicTests.exe starts

    // 1. Setup the Map (BFS/Pathfinding)
    MapData::PrecomputeDistancesAndPaths();

    // 2. Setup the Cards
    CardRegistry::Initialize();

    // --- RUN CATCH2 ---
    // This executes all TEST_CASEs in your project
    int result = Catch::Session().run(argc, argv);

    return result;
}