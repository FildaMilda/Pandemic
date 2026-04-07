#include "Map.h"

uint8_t MapData::cityDistances[NUMBER_OF_CITIES][NUMBER_OF_CITIES];
StaticPath MapData::drivePaths[NUMBER_OF_CITIES][NUMBER_OF_CITIES];

void MapData::PrecomputeDistancesAndPaths()
{
    for (int startNode = 0; startNode < 48; ++startNode) {

        // 1. Initialize distances and the parent tracker
        uint8_t parent[48];
        for (int j = 0; j < 48; j++) {
            cityDistances[startNode][j] = 255; // 255 = Infinity/Unvisited
            parent[j] = 255;
        }

        // 2. Setup BFS
        std::queue<uint8_t> q;
        q.push(startNode);
        cityDistances[startNode][startNode] = 0;
        parent[startNode] = startNode; // Root's parent is itself

        // 3. Run BFS
        while (!q.empty()) {
            uint8_t current = q.front();
            q.pop();

            int count = MapData::neighbor_counts[current];
            for (int i = 0; i < count; i++) {
                uint8_t neighbor = MapData::adjacency[current][i];

                // If unvisited
                if (cityDistances[startNode][neighbor] == 255) {

                    // Set distance
                    cityDistances[startNode][neighbor] = cityDistances[startNode][current] + 1;

                    // Track where we came from to reconstruct the path later!
                    parent[neighbor] = current;

                    q.push(neighbor);
                }
            }
        }

        // 4. Reconstruct and cache the paths for this startNode
        for (int targetNode = 0; targetNode < 48; ++targetNode) {

            // Base case: staying in the same city
            if (startNode == targetNode) {
                drivePaths[startNode][targetNode].length = 0;
                continue;
            }

            // We already know the exact path length from the BFS!
            uint8_t path_len = cityDistances[startNode][targetNode];
            drivePaths[startNode][targetNode].length = path_len;

            // Backtrack from the target to the start
            uint8_t curr = targetNode;
            uint8_t temp_path[48];
            int idx = 0;

            while (curr != startNode) {
                temp_path[idx++] = curr;
                curr = parent[curr];
            }

            // The temp_path is currently backwards (Target -> Start). 
            // We need to reverse it so it goes (Start -> Target)
            for (int i = 0; i < path_len; i++) {
                if (i < 10) { // Safety bound against array overflow
                    drivePaths[startNode][targetNode].nodes[i] = temp_path[path_len - 1 - i];
                }
            }
        }
    }
}
