#ifndef MAP_H
#define MAP_H

#include <cstdint>
#include "Globals.h"

struct MapData {
    // 48 Cities x 8 slots. 
    static const uint8_t adjacency[NUMBER_OF_CITIES][8];
    static const uint8_t neighbor_counts[NUMBER_OF_CITIES];

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
};

#endif