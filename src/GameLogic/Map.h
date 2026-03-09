#ifndef MAP_H
#define MAP_H

#include <cstdint>
#include <bit>
#include <queue>
#include "Globals.h"

struct MapData {
    // 48 Cities x 8 slots. 
    static const uint8_t adjacency[NUMBER_OF_CITIES][8];
    static const uint8_t neighbor_counts[NUMBER_OF_CITIES];
    static uint8_t cityDistances[NUMBER_OF_CITIES][NUMBER_OF_CITIES];

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

    static void PrecomputeDistances();
};

#endif