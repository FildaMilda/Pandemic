#ifndef CARDS_H
#define CARDS_H

#include "Globals.h"

#include <string>
#include <array>
#include <cstdint>
#include <format>

enum CardType : uint8_t {
    TYPE_CITY = 0,
    TYPE_EVENT = 1,
    TYPE_EPIDEMIC = 2
};

struct CardData {
    std::string name;
    CardType type;
    uint8_t cityIndex;
    Color color;

    inline void Print() const {
        std::cout << std::format("Name: {}\nType: {}\nIndex: {}\nColor: {}\n", name, (int)type, (int)cityIndex, (int)color);
    }
};

class CardRegistry {
private:
    static const int registrySize = 64;
    static std::array<CardData, registrySize> registry;

public:
    static void Initialize() {
        uint8_t card_index = 0;

        // Blue cities
        for (int i = 0; i < NUMBER_OF_CITIES_PER_COLOR; i++) {
            registry[card_index] = { BLUE_CITIES[i], TYPE_CITY, card_index, BLUE };
            card_index++;
        }

        // Yellow cities
        for (int i = 0; i < NUMBER_OF_CITIES_PER_COLOR; i++) {
            registry[card_index] = { YELLOW_CITIES[i], TYPE_CITY, card_index, YELLOW };
            card_index++;
        }

        // Black cities
        for (int i = 0; i < NUMBER_OF_CITIES_PER_COLOR; i++) {
            registry[card_index] = { BLACK_CITIES[i], TYPE_CITY, card_index, BLACK };
            card_index++;
        }

        // Red cities
        for (int i = 0; i < NUMBER_OF_CITIES_PER_COLOR; i++) {
            registry[card_index] = { RED_CITIES[i], TYPE_CITY, card_index, RED };
            card_index++;
        }

        // Setup Events
        registry[(uint8_t)EventCardID::Airlift] = { "Airlift",          TYPE_EVENT, 255, NO_COLOR };
        registry[(uint8_t)EventCardID::GovGrant] = { "Gov Grant",        TYPE_EVENT, 255, NO_COLOR };
        registry[(uint8_t)EventCardID::Forecast] = { "Forecast",         TYPE_EVENT, 255, NO_COLOR };
        registry[(uint8_t)EventCardID::OneQuietNight] = { "One Quiet Night",  TYPE_EVENT, 255, NO_COLOR };
        registry[(uint8_t)EventCardID::ResilientPopulation] = { "Resilient Pop",    TYPE_EVENT, 255, NO_COLOR };

        // Epidemic card
        registry[53] = { "Epidemic", TYPE_EPIDEMIC, 255, NO_COLOR };
    }

    static inline bool IsCity(uint8_t cardId) {
        return cardId < NUMBER_OF_CITIES;
    }

    static inline bool IsEvent(uint8_t cardId) {
        return cardId >= NUMBER_OF_CITIES && cardId <= NUMBER_OF_CITIES + NUMBER_OF_EVENT_CARDS - 1;
    }

    static inline bool IsEpidemic(uint8_t cardId) {
        return cardId == NUMBER_OF_CITIES + NUMBER_OF_EVENT_CARDS;
    }

    static inline uint8_t GetCityIndex(uint8_t cardId) {
        if (cardId >= NUMBER_OF_CITIES) return 255;
        return cardId;
    }

    static inline Color GetColor(uint8_t cardId) {
        return registry[cardId].color;
    }

    static const std::string& GetName(uint8_t cardId) {
        if (cardId > NUMBER_OF_UNIQUE_CARDS) return "INVALID";
        return registry[cardId].name;
    }

    static const void PrintInfo(uint8_t cardId) {
        registry[cardId].Print();
    }

    static const uint8_t GetEpidemicCardID() {
        return PLAYER_DECK_SIZE; // last card
    }

    static const void DebugPrint();
};

#endif