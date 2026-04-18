#ifndef PLAYER_H
#define PLAYER_H

#include <cstdint>
#include <iostream>
#include <bit>
#include <string>
#include <array>

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
        packed_locations = 0; // All start Atlanta (index=0)

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

    inline uint8_t GetOwnerOf(uint8_t card_id) const {
        uint64_t card_mask = 1ULL << card_id;

        for (uint8_t i = 0; i < count; ++i) {
            if (hands[i] & card_mask) {
                return i;
            }
        }
        return 255;
    }

    inline int GetColorCount(uint8_t player_id, ColorType color) const {
        return std::popcount(hands[player_id] & GameConstants::COLOR_MASKS[(int)color]);
    }

    // Returns the color the player can cure.
    inline ColorType GetCureColor(uint8_t player_id) const {
        uint64_t hand = hands[player_id];
        int threshold = (GetRole(player_id) == Role::Scientist) ? 4 : 5;

        // We check all 4 colors using the masks
        for (int i = 0; i < 4; ++i) {
            // 1. Mask out everything except the color we are checking
            // 2. Count the remaining set bits instantly
            if (std::popcount(hand & GameConstants::COLOR_MASKS[i]) >= threshold) {
                return static_cast<ColorType>(i);
            }
        }
        return ColorType::NO_COLOR;
    }

    // Returns true if any of the players has the event card
    // and is therefore usable.
    inline bool DoPlayersHaveEventCard(uint8_t event_id) const {
        uint64_t event_mask = 1ULL << event_id;
        uint64_t all_hands = hands[0] | hands[1] | hands[2] | hands[3];
        return (all_hands & event_mask) != 0;
    }

    inline bool IsNeededForCure(uint8_t player_id, ColorType color, uint8_t card_id) const {
        if (CardRegistry::GetColor(card_id) != color) {
            return false;
        }

        uint64_t hand = hands[player_id];
        int count = std::popcount(hand & GameConstants::COLOR_MASKS[color]);

        int threshold = (GetRole(player_id) == Role::Scientist) ? 4 : 5;

        return count <= threshold;
    }

    inline bool CanCharter(uint8_t player_id) const {
        uint64_t charter_mask = 1ULL << GetLocation(player_id);
        return (hands[player_id] & charter_mask) != 0;
    }

    inline bool HasRole(uint8_t player_id, Role role) const {
        return roles[player_id] == role;
    }

    inline uint8_t FindRole(Role role) const {
        for (int i = 0; i < count; i++) {
            if (GetRole(i) == role) return i;
        }
        return 255;
    }

    void Print() const;
};

#endif