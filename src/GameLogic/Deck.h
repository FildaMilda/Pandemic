#ifndef DECK_H
#define DECK_H

#include <cstdint>
#include <algorithm>
#include <cstring>
#include <random>

#include "Cards.h"

template <int TSize>
struct StackDeck {
    uint8_t cards[TSize];

    // Points to the index JUST PAST the top card.
    // If top_index == 10, it means cards[0]...cards[9] are in the deck.
    // cards[10]...cards[TSize-1] are in the discard pile.
    uint8_t top_index;
    uint8_t discard_index;

    void Init() {
        top_index = 0;
        discard_index = TSize;
    }

    struct ConstDeckRange {
        const uint8_t* b;
        const uint8_t* e;
        const uint8_t* begin() const { return b; }
        const uint8_t* end() const { return e; }
    };

    // Returns a loopable range for the Draw Pile (Bottom to Top)
    inline ConstDeckRange GetDrawPile() const {
        return ConstDeckRange{ &cards[0], &cards[top_index] };
    }

    // Returns a loopable range for the Discard Pile (Top to Bottom)
    inline ConstDeckRange GetDiscardPile() const {
        return ConstDeckRange{ &cards[discard_index], &cards[TSize] };
    }

    // Used during setup to populate the deck
    inline void AddCard(uint8_t cardId) {
        if (top_index < TSize) {
            cards[top_index++] = cardId;
        }
    }

    // Returns the Top Card and moves the pointer down.
    // The card remains in memory (in the "Discarded" zone).
    /*
    inline uint8_t Draw() {
        if (top_index == 0) return 255; // Empty
        return cards[--top_index];
    }
    */

    inline uint8_t DrawAndRemove() {
        if (top_index == 0) return 255;
        top_index--;
        uint8_t card = cards[top_index];
        cards[top_index] = 255;
        return card;
    }

    inline uint8_t DrawAndDiscard() {
        if (top_index == 0) return 255;
        top_index--;
        uint8_t card = cards[top_index];
        cards[top_index] = 255;
        discard_index--;
        cards[discard_index] = card;
        return card;
    }

    inline void AddToDiscard(uint8_t cardId) {
        if (discard_index > top_index) {
            discard_index--;
            cards[discard_index] = cardId;
        }
    }

    // Used only for resolving Epidemic Card
    inline uint8_t DrawBottomAndDiscard() {
        if (top_index == 0) return 255;
        uint8_t bottomCard = cards[0];

        for (int i = 0; i < top_index - 1; i++) {
            cards[i] = cards[i + 1];
        }

        top_index--;
        cards[top_index] = 255;

        discard_index--;
        cards[discard_index] = bottomCard;

        return bottomCard;
    }

    // Just look, don't touch (For Forecast / Contingency Planner)
    inline uint8_t Peek(int depth = 0) const {
        if (top_index <= depth) return 255;
        return cards[top_index - 1 - depth];
    }

    inline int Count() const { return top_index; }

    // Shuffles ONLY the current Draw Pile (0 to top_index)
    void Shuffle(std::mt19937* rng) {
        std::shuffle(cards, cards + top_index, *rng);
    }

    inline void Print() const {
        int drawCount = top_index;
        int discardCount = TSize - discard_index;
        int unusedCount = discard_index - top_index;

        std::cout << std::format("=== Deck Memory Map (Capacity: {}) ===\n", TSize);
        std::cout << std::format("Draw: {} | Limbo/Hands: {} | Discard: {}\n", drawCount, unusedCount, discardCount);

        std::cout << "--- Discard Pile (Top to Bottom) ---\n";
        if (discardCount == 0) {
            std::cout << "  (Empty)\n";
        }
        else {
            // Top of the discard pile is at discard_index. 
            // Bottom of the discard pile is at TSize - 1.
            for (int i = discard_index; i < TSize; i++) {
                std::cout << std::format("  [{:02d}] ID: {:02d} - {}\n", i, cards[i], CardRegistry::GetName(cards[i]));
            }
        }

        std::cout << "--- Unused / In Player Hands (Limbo) ---\n";
        if (unusedCount == 0) {
            std::cout << "  (None)\n";
        }
        else {
            std::cout << std::format("  ... {} empty slots (255) from index {} to {} ...\n",
                unusedCount, top_index, discard_index - 1);

            // Optional: If you want to literally print every 255 slot, uncomment this:
            /*
            for (int i = discard_index - 1; i >= top_index; i--) {
                std::cout << std::format("  [{:02d}] EMPTY (255)\n", i);
            }
            */
        }

        std::cout << "--- Draw Pile (Top to Bottom) ---\n";
        if (drawCount == 0) {
            std::cout << "  (Empty)\n";
        }
        else {
            // Top of the draw pile is at top_index - 1.
            // Bottom of the draw pile is at 0.
            for (int i = top_index - 1; i >= 0; i--) {
                std::cout << std::format("  [{:02d}] ID: {:02d} - {}\n", i, cards[i], CardRegistry::GetName(cards[i]));
            }
        }
        std::cout << "======================================\n";
    }
};

struct PlayerDeck : public StackDeck<64> {
    // ?Might use 64 instead of 60+-=PLAYER_DECK_SIZE to keep it aligned to cache lines?
   
    void Init(std::mt19937* rng) {
        top_index = PLAYER_DECK_SIZE;
        discard_index = 64;
        for (uint8_t i = 0; i < 64; i++) {
            if (i < top_index) cards[i] = i;
            else cards[i] = 255;
        }
        Shuffle(rng);
    }

    // Insert Logic: Epidemic cards need to be inserted at specific intervals.
    // This O(N) operation happens only during setup.
    void InsertAt(int index, uint8_t cardId) {
        if (top_index >= 64) return;
        if (index > top_index) index = top_index;

        // Shift elements up to make a hole
        std::memmove(&cards[index + 1], &cards[index], top_index - index);
        cards[index] = cardId;
        top_index++;
    }
};

struct InfectionDeck : public StackDeck<INFECTION_DECK_SIZE> {

    // --- THE "INTENSIFY" LOGIC (Epidemic Step) ---
    // "Shuffle the Discard Pile and put it on TOP of the Draw Pile."
    // In our structure, the "Discard Pile" is cards[top_index...47].

    void Init(std::mt19937 *rng) {
        top_index = INFECTION_DECK_SIZE;
        discard_index = INFECTION_DECK_SIZE;
        for (uint8_t i = 0; i < INFECTION_DECK_SIZE; i++) {
            cards[i] = i;
        }
        Shuffle(rng);
    }

    void RemoveFromDiscardPile(uint8_t cityId) {
        int foundIndex = -1;

        for (int i = discard_index; i < INFECTION_DECK_SIZE; i++) {
            if (cards[i] == cityId) {
                foundIndex = i;
                break;
            }
        }

        if (foundIndex != -1) {
            for (int i = foundIndex; i > discard_index; i--) {
                cards[i] = cards[i - 1];
            }

            cards[discard_index] = 255;
            discard_index++;
        }
    }

    inline void ResolveForecast(uint8_t idx0, uint8_t idx1, uint8_t idx2, uint8_t idx3, uint8_t idx4, uint8_t idx5) {

        // Safety check: Usually there are 6 cards, but we protect against underflow
        // just in case Forecast is played at the very end of the game.
        int num_cards = std::min((int)top_index, 6);
        if (num_cards < 6) {
            return;
        }

        // 1. Copy the original top 6 cards into a temporary buffer.
        uint8_t orig[6];
        orig[0] = cards[top_index - 1];
        orig[1] = cards[top_index - 2];
        orig[2] = cards[top_index - 3];
        orig[3] = cards[top_index - 4];
        orig[4] = cards[top_index - 5];
        orig[5] = cards[top_index - 6];

        // 2. Map them to their new positions in the actual deck.
        cards[top_index - 1 - idx0] = orig[0];
        cards[top_index - 1 - idx1] = orig[1];
        cards[top_index - 1 - idx2] = orig[2];
        cards[top_index - 1 - idx3] = orig[3];
        cards[top_index - 1 - idx4] = orig[4];
        cards[top_index - 1 - idx5] = orig[5];
    }

    inline void Intensify(std::mt19937* rng) {
        int dCount = INFECTION_DECK_SIZE - discard_index;

        // Safety check: if discard pile is empty, do nothing!
        if (dCount == 0) return;

        // 1. Shuffle the discard pile in place
        // (From the top of the discard pile to the end of the array)
        std::shuffle(cards + discard_index, cards + INFECTION_DECK_SIZE, *rng);

        // 2. Slide the shuffled discard block down to sit on top of the draw pile.
        // dest: cards + top_index
        // src:  cards + discard_index
        // size: dCount * 1 byte
        std::memmove(cards + top_index, cards + discard_index, dCount * sizeof(uint8_t));

        // 3. Update the draw pile pointer (it just grew by dCount)
        top_index += dCount;

        // 4. Clean up the remaining memory. (this step is optional I think)
        for (int i = top_index; i < INFECTION_DECK_SIZE; i++) {
            cards[i] = 255;
        }

        // 5. Reset the discard pointer (the discard pile is now completely empty)
        discard_index = INFECTION_DECK_SIZE;
    }
};

struct Decks {
    PlayerDeck player_deck;       
    InfectionDeck infection_deck;
};

#endif