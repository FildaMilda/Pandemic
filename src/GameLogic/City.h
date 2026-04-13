#ifndef CITY_H
#define CITY_H

#include "Globals.h"
#include "Map.h"
#include "Cards.h"

#include <cstdint>
#include <iostream>
#include <cstring>
#include <iomanip>

struct CityNode {
    uint8_t blue_cubes : 2;
    uint8_t yellow_cubes : 2;
    uint8_t black_cubes : 2;
    uint8_t red_cubes : 2;

    inline uint8_t GetCubes(ColorType colorIdx) const {
        /*
        We use a switch because bitfields cannot be accessed via array index directly
        */
        switch (colorIdx) {
        case ColorType::BLUE:   return blue_cubes;
        case ColorType::YELLOW: return yellow_cubes;
        case ColorType::BLACK:  return black_cubes;
        case ColorType::RED:    return red_cubes;
        default:     return 0;
        }
    }

    inline bool AddCube(ColorType colorIdx) {
        /*
        Add a cube safely (Max 3)
        Returns true if an OUTBREAK would occur (count > 3)
        */
        switch (colorIdx) {
        case ColorType::BLUE:
            if (blue_cubes < 3) { blue_cubes++; return false; }
            return true;
        case ColorType::YELLOW:
            if (yellow_cubes < 3) { yellow_cubes++; return false; }
            return true;
        case ColorType::BLACK:
            if (black_cubes < 3) { black_cubes++; return false; }
            return true;
        case ColorType::RED:
            if (red_cubes < 3) { red_cubes++; return false; }
            return true;
        }
        return false;
    }

    inline bool RemoveCube(ColorType colorIdx) {
        switch (colorIdx) {
        case ColorType::BLUE:   if (blue_cubes > 0) { blue_cubes--; return true; } break;
        case ColorType::YELLOW: if (yellow_cubes > 0) { yellow_cubes--; return true; } break;
        case ColorType::BLACK:  if (black_cubes > 0) { black_cubes--; return true; } break;
        case ColorType::RED:    if (red_cubes > 0) { red_cubes--; return true; } break;
        }
        return false;
    }

    inline uint8_t RemoveAllCubes(ColorType colorIdx) {
        uint8_t removed = 0;
        switch (colorIdx) {
        case ColorType::BLUE:   removed = blue_cubes; blue_cubes = 0; break;
        case ColorType::YELLOW: removed = yellow_cubes; yellow_cubes = 0; break;
        case ColorType::BLACK:  removed = black_cubes; black_cubes = 0; break;
        case ColorType::RED:    removed = red_cubes; red_cubes = 0; break;
        }
        return removed;
    }
};

struct CityState {
    // 48 Cities * 2 Bytes = 96 Bytes Total
    uint64_t outbreak_flag;
    uint64_t station_mask;
    uint64_t hotspot_mask;
    uint8_t global_cubes[4];
    CityNode cities[NUMBER_OF_CITIES];

    void Init() {
        std::memset(cities, 0, sizeof(cities));
        std::memset(global_cubes, 0, sizeof(global_cubes));
        station_mask = 0;
        // Add station to Atlanta
        AddStation(0);
        outbreak_flag = 0;
        hotspot_mask = 0;
    }

    inline bool AddDisease(uint8_t cityIndex, ColorType color) {
        bool isOutbreak = cities[cityIndex].AddCube(color);
        if (!isOutbreak) {
            global_cubes[color]++;
            UpdateHotspotBit(cityIndex);
        }
        return isOutbreak;
    }

    inline void AddDiseases(uint8_t cityIndex, ColorType color, uint8_t count) {
        for (uint8_t i = 0; i < count; i++) AddDisease(cityIndex, color);
    }

    inline void AddStation(uint8_t cityId) {
        station_mask |= (1ULL << cityId);
    }

    inline void RemoveStation(uint8_t cityId) {
        station_mask &= ~(1ULL << cityId);
    }

    inline void RemoveDisease(uint8_t cityId, ColorType color) {
        if (cities[cityId].RemoveCube(color)) {
            global_cubes[color]--;
            UpdateHotspotBit(cityId);
        }
    }

    inline void RemoveAllDiseases(uint8_t cityId, ColorType color) {
        uint8_t removed = cities[cityId].RemoveAllCubes(color);
        global_cubes[color] -= removed;
        if (removed > 0) {
            UpdateHotspotBit(cityId);
        }
    }

    inline bool HasDisease(uint8_t cityId, ColorType color) const {
        return cities[cityId].GetCubes(color) > 0;
    }

    inline bool HasDisease(uint8_t city_id) const {
        return cities[city_id].GetCubes(ColorType::BLACK) + cities[city_id].GetCubes(ColorType::BLUE)
            + cities[city_id].GetCubes(ColorType::RED) + cities[city_id].GetCubes(ColorType::YELLOW) > 0;
    }

    inline uint8_t GetDiseaseCount(uint8_t city_id, ColorType color) const {
        return cities[city_id].GetCubes(color);
    }

    inline bool HasStation(uint8_t cityId) const {
        return (station_mask & (1ULL << cityId)) != 0;
    }

    inline uint64_t GetStationMask() const {
        return station_mask;
    }

    inline uint64_t GetHotspotMask() const {
        return hotspot_mask;
    }

    inline uint8_t GetStationCount() const {
        return std::popcount(station_mask);
    }

    inline uint8_t GetHotspotCount() const {
        return std::popcount(hotspot_mask);
    }

    inline bool HasHotspot(uint8_t city_id) const {
        return (hotspot_mask & (1ULL << city_id)) != 0;
    }

    inline void SetOutbroken(uint8_t cityId) {
        outbreak_flag |= (1ULL << cityId);
    }

    inline bool HasOutbroken(uint8_t cityId) const {
        return (outbreak_flag & (1ULL << cityId)) != 0;
    }

    inline void ClearOutbreakChain() {
        outbreak_flag = 0;
    }

    inline bool HasLostToCubes() const {
        return global_cubes[ColorType::RED] > MAX_NUMBER_OF_CUBES_PER_COLOR || global_cubes[ColorType::BLACK] > MAX_NUMBER_OF_CUBES_PER_COLOR
            || global_cubes[ColorType::BLUE] > MAX_NUMBER_OF_CUBES_PER_COLOR || global_cubes[ColorType::YELLOW] > MAX_NUMBER_OF_CUBES_PER_COLOR;
    }

    inline uint8_t GetTotalCubeCount(ColorType color) const {
        return global_cubes[color];
    }

    inline uint8_t GetCubeCount(uint8_t city_id, ColorType color) const {
        return cities[city_id].GetCubes(color);
    }

    inline uint8_t GetCubeCount(uint8_t city_id) const {
        return cities[city_id].GetCubes(ColorType::BLACK) + cities[city_id].GetCubes(ColorType::BLUE)
            + cities[city_id].GetCubes(ColorType::RED) + cities[city_id].GetCubes(ColorType::YELLOW);
    }

    inline uint8_t GetTotalCubeCount(uint8_t city_id) const {
        return GetCubeCount(city_id, ColorType::BLACK) + GetCubeCount(city_id, ColorType::BLUE)
            + GetCubeCount(city_id, ColorType::YELLOW) + GetCubeCount(city_id, ColorType::RED);
    }

    inline void UpdateHotspotBit(uint8_t cityIndex) {
        // A city is a hotspot if it has exactly 3 cubes of ANY color
        bool isHot = (cities[cityIndex].GetCubes(ColorType::BLUE) >= 3) ||
            (cities[cityIndex].GetCubes(ColorType::YELLOW) >= 3) ||
            (cities[cityIndex].GetCubes(ColorType::BLACK) >= 3) ||
            (cities[cityIndex].GetCubes(ColorType::RED) >= 3);

        if (isHot) {
            hotspot_mask |= (1ULL << cityIndex);
        }
        else {
            hotspot_mask &= ~(1ULL << cityIndex);
        }
    }

    inline uint8_t GetDistanceToNearestStation(uint8_t playerLocation) const {
        return MapData::GetDistanceToNearest(playerLocation, station_mask);
    }

    inline uint8_t GetNearestStation(uint8_t city_id) const {
        uint8_t closest_station = 255;
        uint64_t stations = GetStationMask();
        uint8_t dist = GetDistanceToNearestStation(city_id);
        while (stations > 0) {
            uint8_t st = std::countr_zero(stations);
            if (MapData::cityDistances[city_id][st] == dist) {
                closest_station = st;
                break;
            }
            stations &= (stations - 1);
        }
        return closest_station;
    }

    inline uint8_t GetDistanceToNearestHotspot(uint8_t playerLocation) const {
        return MapData::GetDistanceToNearest(playerLocation, hotspot_mask);
    }

    inline ColorType GetDominantDiseaseColor(uint8_t city_id) const {
        int total_cubes = GetCubeCount(city_id);
        if (total_cubes == 0) return ColorType::NO_COLOR;

        ColorType native_color = CardRegistry::GetColor(city_id);
        int native_count = GetCubeCount(city_id, native_color);

        if (native_count == total_cubes) {
            return native_color;
        }

        ColorType best_color = native_color;
        int max_count = native_count;

        for (int c = 0; c < ColorType::COUNT; ++c) {
            if (c == native_color) continue;

            int count = GetCubeCount(city_id, static_cast<ColorType>(c));
            if (count > max_count) {
                max_count = count;
                best_color = static_cast<ColorType>(c);
            }
        }

        return best_color;
    }

    void Print() const;
};


#endif