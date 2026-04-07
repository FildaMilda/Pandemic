#ifndef MAP_H
#define MAP_H

#include <cstdint>
#include <bit>
#include <queue>
#include "Globals.h"

const int LONGEST_DRIVE = 9;

struct StaticPath {
    uint8_t length;
    uint8_t nodes[LONGEST_DRIVE];
};

struct MapData {

    static inline constexpr uint8_t neighbor_counts[NUMBER_OF_CITIES] = {
        3, 4, 5, 3, 4, 4, 4, 5, 5, 4, 3, 3, // 0-11
        4, 5, 4, 5, 3, 1, 2, 4, 3, 4, 3, 2, // 12-23
        4, 4, 6, 3, 3, 5, 4, 5, 4, 3, 4, 5, // 24-35
        2, 5, 3, 4, 2, 4, 5, 5, 4, 5, 3, 3  // 36-47
    };
    static inline constexpr uint8_t adjacency[NUMBER_OF_CITIES][8] = {
        { 2, 5, 14, 255, 255, 255, 255, 255 },    // 0: Atlanta
        { 2, 12, 39, 45, 255, 255, 255, 255 },    // 1: San Francisco
        { 1, 12, 13, 0, 3, 255, 255, 255 },       // 2: Chicago
        { 2, 4, 5, 255, 255, 255, 255, 255 },     // 3: Montreal
        { 3, 5, 6, 7, 255, 255, 255, 255 },       // 4: New York
        { 3, 4, 0, 14, 255, 255, 255, 255 },      // 5: Washington
        { 4, 7, 8, 9, 255, 255, 255, 255 },       // 6: London
        { 4, 6, 8, 19, 24, 255, 255, 255 },       // 7: Madrid
        { 6, 7, 9, 10, 24, 255, 255, 255 },       // 8: Paris
        { 6, 8, 10, 11, 255, 255, 255, 255 },     // 9: Essen
        { 8, 9, 26, 255, 255, 255, 255, 255 },    // 10: Milan
        { 9, 26, 28, 255, 255, 255, 255, 255 },   // 11: St. Petersburg
        { 1, 2, 13, 46, 255, 255, 255, 255 },     // 12: Los Angeles
        { 12, 2, 14, 15, 16, 255, 255, 255 },     // 13: Mexico City
        { 5, 0, 13, 15, 255, 255, 255, 255 },     // 14: Miami
        { 13, 14, 16, 19, 18, 255, 255, 255 },    // 15: Bogota
        { 13, 15, 17, 255, 255, 255, 255, 255 },  // 16: Lima
        { 16, 255, 255, 255, 255, 255, 255, 255 },// 17: Santiago
        { 15, 19, 255, 255, 255, 255, 255, 255 }, // 18: Buenos Aires
        { 15, 18, 7, 20, 255, 255, 255, 255 },    // 19: Sao Paulo
        { 19, 22, 21, 255, 255, 255, 255, 255 },  // 20: Lagos
        { 20, 22, 23, 25, 255, 255, 255, 255 },   // 21: Khartoum
        { 20, 21, 23, 255, 255, 255, 255, 255 },  // 22: Kinshasa
        { 22, 21, 255, 255, 255, 255, 255, 255 }, // 23: Johannesburg
        { 7, 8, 26, 25, 255, 255, 255, 255 },     // 24: Algiers
        { 24, 29, 21, 27, 255, 255, 255, 255 },   // 25: Cairo
        { 10, 24, 25, 29, 28, 11, 255, 255 },     // 26: Istanbul
        { 25, 29, 31, 255, 255, 255, 255, 255 },  // 27: Riyadh
        { 11, 26, 30, 255, 255, 255, 255, 255 },  // 28: Moscow
        { 26, 30, 25, 31, 27, 255, 255, 255 },    // 29: Baghdad
        { 28, 29, 31, 32, 255, 255, 255, 255 },   // 30: Tehran
        { 27, 29, 30, 32, 33, 255, 255, 255 },    // 31: Karachi
        { 30, 31, 33, 34, 255, 255, 255, 255 },   // 32: Delhi
        { 31, 32, 35, 255, 255, 255, 255, 255 },  // 33: Mumbai
        { 32, 35, 43, 42, 255, 255, 255, 255 },   // 34: Kolkata
        { 33, 32, 34, 43, 44, 255, 255, 255 },    // 35: Chennai
        { 37, 38, 255, 255, 255, 255, 255, 255 }, // 36: Beijing
        { 36, 38, 39, 41, 42, 255, 255, 255 },    // 37: Shanghai
        { 36, 37, 39, 255, 255, 255, 255, 255 },  // 38: Seoul
        { 38, 37, 40, 1, 255, 255, 255, 255 },    // 39: Tokyo
        { 39, 41, 255, 255, 255, 255, 255, 255 }, // 40: Osaka
        { 40, 37, 42, 45, 255, 255, 255, 255 },   // 41: Taipei
        { 37, 41, 34, 43, 45, 255, 255, 255 },    // 42: Hong Kong
        { 34, 42, 35, 44, 47, 255, 255, 255 },    // 43: Bangkok
        { 35, 43, 47, 46, 255, 255, 255, 255 },   // 44: Jakarta
        { 1, 41, 42, 47, 46, 255, 255, 255 },     // 45: Manila
        { 12, 44, 45, 255, 255, 255, 255, 255 },  // 46: Sydney
        { 43, 44, 45, 255, 255, 255, 255, 255 }   // 47: Ho Chi Minh City
    };

    static uint8_t cityDistances[NUMBER_OF_CITIES][NUMBER_OF_CITIES];
    static StaticPath drivePaths[NUMBER_OF_CITIES][NUMBER_OF_CITIES];

    static inline const uint8_t* GetNeighbors(uint8_t cityId) {
        return &adjacency[cityId][0];
    }

    static inline int GetNeighborCount(uint8_t cityId) {
        return neighbor_counts[cityId];
    }

    static inline bool IsNeighbor(uint8_t cityA, uint8_t targetCity) {
        int count = neighbor_counts[cityA];
        const uint8_t* neighbors = adjacency[cityA];

        for (int i = 0; i < count; i++) {
            if (neighbors[i] == targetCity) {
                return true;
            }
        }
        return false;
    }
    static inline uint8_t GetDistance(uint8_t a, uint8_t b) {
        return cityDistances[a][b];
    }

    static inline uint8_t GetDistanceToNearest(uint8_t city_id, uint64_t bitmask) {
        // Already at the location
        //if ((bitmask >> city_id) & 1ULL) return 0;

        uint8_t minDistance = 255;
        uint64_t temp_mask = bitmask;

        while (temp_mask > 0) {
            int CityId = std::countr_zero(temp_mask);

            uint8_t d = MapData::GetDistance(city_id, (uint8_t)CityId);
            if (d < minDistance) minDistance = d;

            temp_mask &= (temp_mask - 1);
        }
        return minDistance;
    }

    static inline uint64_t GetNeighborsMask(uint8_t cityId) {
        uint64_t mask = 0;
        int count = neighbor_counts[cityId];
        const uint8_t* neighbors = adjacency[cityId];

        for (int i = 0; i < count; i++) {
            mask |= (1ULL << neighbors[i]);
        }

        return mask;
    }

    static void PrecomputeDistancesAndPaths();
};

#endif