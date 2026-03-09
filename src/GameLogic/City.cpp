#include "City.h"

void CityState::Print() const
{
    std::cout << "\n==================== GLOBAL CITY STATE ====================\n";
    std::cout << std::left
        << std::setw(4) << "ID"
        << std::setw(18) << "City Name"
        << std::setw(8) << "Blue"
        << std::setw(8) << "Yellow"
        << std::setw(8) << "Black"
        << std::setw(8) << "Red"
        << "Station\n";
    std::cout << "-----------------------------------------------------------\n";

    for (int i = 0; i < NUMBER_OF_CITIES; i++) {
        const CityNode& city = cities[i];

        // if (city.blue_cubes > 0 || city.yellow_cubes > 0 || 
        //     city.black_cubes > 0 || city.red_cubes > 0 || city.has_station) {

        std::cout << std::left
            << "[" << std::setw(2) << i << "] "
            << std::setw(18) << CardRegistry::GetName(i)
            << std::setw(8) << (int)city.blue_cubes
            << std::setw(8) << (int)city.yellow_cubes
            << std::setw(8) << (int)city.black_cubes
            << std::setw(8) << (int)city.red_cubes
            << (HasStation(i) ? "[X]" : "   ")
            << "\n";
        // }
    }
    std::cout << "===========================================================\n\n";
}
