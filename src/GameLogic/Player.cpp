#include "Player.h"

void Players::Print() const {
    std::cout << "--- Player State (Count: " << (int)count << ") ---\n";

    for (int i = 0; i < count; ++i) {
        std::cout << "Player " << i << ":\n";

        // 1. Role (Displaying as integer; you might want a string lookup table later)
        std::cout << "  Role:     " << (roles[i] == 255 ? "None" : std::to_string(roles[i])) << "\n";

        // 2. Location (Extracted from packed_locations)
        std::cout << "  Location: " << (int)GetLocation(i) << "\n";

        // 3. Hand (Iterate through bits of the uint64_t)
        std::cout << "  Cards:    [ ";
        bool first = true;
        uint64_t hand = hands[i];

        for (int cardId = 0; cardId < 64; ++cardId) {
            if ((hand >> cardId) & 1ULL) {
                if (!first) std::cout << ", ";
                std::cout << cardId;
                first = false;
            }
        }

        if (first) std::cout << "Empty";
        std::cout << " ]\n";
        std::cout << "---------------------------\n";
    }
}