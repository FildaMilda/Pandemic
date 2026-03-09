#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <iostream>
#include <bit>
#include <string>
#include "Globals.h"
#include "Cards.h"

struct ColorCount {
    int count;
    ColorType color;
};

struct Players {
    // HANDS: 4 Players * 8 Bytes = 32 Bytes
    uint64_t hands[4];

    // LOCATIONS: 4 Players * 6 bits = 24 bits.
    // We store this in one 32-bit integer (4 Bytes).
    // Layout: [ Unused(8) | P3(6) | P2(6) | P1(6) | P0(6) ]
    uint32_t packed_locations;

    // ROLES: 4 Players * 1 Byte = 4 Bytes.
    uint8_t roles[4];

    uint8_t count;

    void Init(uint8_t num_players) {
        count = num_players;
        packed_locations = 0;

        for (int i = 0; i < 4; i++) {
            hands[i] = 0;
            roles[i] = 255; // No role
        }
    }

    inline void AddCard(int playerIdx, int cardId) {
        hands[playerIdx] |= (1ULL << cardId);
    }

    inline void RemoveCard(int playerIdx, int cardId) {
        hands[playerIdx] &= ~(1ULL << cardId);
    }

    inline bool HasCard(int playerIdx, int cardId) const {
        return (hands[playerIdx] >> cardId) & 1ULL;
    }

    inline int GetHandSize(int playerIdx) const {
        return std::popcount(hands[playerIdx]);
    }

    inline void TransferCard(int fromIdx, int toIdx, int cardId) {
        RemoveCard(fromIdx, cardId);
        AddCard(toIdx, cardId);
    }

    inline uint8_t GetLocation(int playerIdx) const {
        // Shift right by (playerIdx * 6) and mask the bottom 6 bits
        return (packed_locations >> (playerIdx * 6)) & 0x3F;
    }

    inline void SetLocation(int playerIdx, int cityId) {
        // 1. Create a "Hole" (Zero out the old bits)
        // If we want to write to P1 (bits 6-11), we need a mask like ...111000000111...
        int shift = playerIdx * 6;
        uint32_t clear_mask = ~(0x3F << shift);
        packed_locations &= clear_mask;

        // 2. Fill the "Hole" (OR in the new bits)
        packed_locations |= (static_cast<uint32_t>(cityId & 0x3F) << shift);
    }

    inline bool AreTogether(int p1, int p2) const {
        return GetLocation(p1) == GetLocation(p2);
    }

    inline void SetPlayerCount(uint8_t playerCount) {
        count = playerCount;
    }

    inline Role GetRole(uint8_t playerId) const {
        return (Role)roles[playerId];
    }

    inline bool IsAnyPlayerAt(int cityId) const {
        for (int i = 0; i < count; i++) {
            if (GetLocation(i) == cityId) {
                return true;
            }
        }
        return false;
    }

    inline ColorCount GetMostFrequentColor(uint8_t player_id) const {
        uint64_t hand = hands[player_id];

        int maxCount = 0;
        ColorType bestColor = ColorType::NO_COLOR;

        for (int i = 0; i < 4; ++i) {
            int currentCount = std::popcount(hand & GameConstants::COLOR_MASKS[i]);

            if (currentCount > maxCount) {
                maxCount = currentCount;
                bestColor = (ColorType)(i);
            }
        }

        return { maxCount, bestColor };
    }

    inline ColorCount GetLeastFrequentColor(uint8_t player_id) const {
        // Returns only the colors that the player HAS. 
        // If player has zero black cards, we don't count that

        uint64_t hand = hands[player_id];

        int minCount = 100;
        ColorType bestColor = ColorType::NO_COLOR;

        for (int i = 0; i < 4; ++i) {
            int currentCount = std::popcount(hand & GameConstants::COLOR_MASKS[i]);

            if (currentCount != 0 && currentCount < minCount) {
                minCount = currentCount;
                bestColor = (ColorType)(i);
            }
        }

        return { minCount, bestColor };
    }

    void Print() const;
};

#endif