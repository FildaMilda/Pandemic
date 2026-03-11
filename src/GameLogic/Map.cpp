#include "Map.h"

uint8_t MapData::cityDistances[NUMBER_OF_CITIES][NUMBER_OF_CITIES];

void MapData::PrecomputeDistances()
{
    
    for (int startNode = 0; startNode < 48; ++startNode) {
        // Initialize with a "infinity" value
        for (int j = 0; j < 48; j++) cityDistances[startNode][j] = 255;

        // BFS
        std::queue<uint8_t> q;
        q.push(startNode);
        cityDistances[startNode][startNode] = 0;

        while (!q.empty()) {
            uint8_t current = q.front();
            q.pop();

            int count = MapData::neighbor_counts[current];
            for (int i = 0; i < count; i++) {
                uint8_t neighbor = MapData::adjacency[current][i];
                if (cityDistances[startNode][neighbor] == 255) {
                    cityDistances[startNode][neighbor] = cityDistances[startNode][current] + 1;
                    q.push(neighbor);
                }
            }
        }
    }
    
}
