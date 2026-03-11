#ifndef ACTION_H
#define ACTION_H

#include "Globals.h"
#include <iostream>

enum ActionType : uint8_t {
    DRIVE = 0,          
    DIRECT_FLIGHT = 1,  
    CHARTER_FLIGHT = 2, 
    SHUTTLE_FLIGHT = 3, 
    BUILD = 4,          
    TREAT = 5,          
    SHARE = 6,          
    CURE = 7,

    PLANNER_TAKE = 8,           // Contingency Planner taking an event card
    DISPATCHER_MOVE = 9,        // Dispatcher moving pawn to a pawn
    DISPATCHER_MOVE_AS = 10,    // Dispatcher moving as another player
    EXPERT_BUILD = 11,          // Building Station as Operations Expert
    EXPERT_MOVE = 12,           // Moving from research station 

    GOVERNMENT_GRANT = 13,
    FORECAST = 14,
    RESILIENT_POPULATION = 15,
    ONE_QUIET_NIGHT = 16,
    AIRLIFT = 17,

    DISCARD_CARD = 18,
    REMOVE_STATION = 19,

    END_TURN = 20,
    PLANNER_USE
};

struct Action {
    union {
        uint32_t raw_data;

        struct {
            uint32_t type : 5;
        } base;

        // (1) ACTION TYPE (move)
        // One city/card (number 0-47) - 6 bits
        // Player index (number 0-3) - 2 bits
        // Drive, Flights, etc.
        struct {
            uint32_t type : 5;
            uint32_t target_city : 6;
            uint32_t executing_player_id : 2;
            uint32_t target_player_id : 2;
            uint32_t _unused : 17;
        } move;

        // (2) ACTION TYPE (treat)
        // One city/card - 6 bits
        // Player index - 2 bits
        // Color (number 0-3) - 2 bits
        struct {
            uint32_t type : 5;
            uint32_t target_city : 6;
            uint32_t player_id : 2;
            uint32_t color_id : 2;
            uint32_t _unused : 17;
        } treat;

        // (3) ACTION TYPE (share)
        // One city/card - 6 bits
        // Player index - 2 bits (doing the action)
        // Giving/Taking the card - 1 bit
        // Player index - 2 bits (player receiving the card)
        // Used only for Share Knowledge action
        struct {
            uint32_t type : 5;
            uint32_t target_city : 6;
            uint32_t player_id : 2;
            uint32_t is_giving : 1;
            uint32_t receiving_player_id : 2;
            uint32_t _unused : 16;
        } share;

        // (4) ACTION TYPE (discover_cure)
        // Index of the colored card in the *_CITIES arrays in Globals.h
        // Used for Dicovering a Cure
        struct {
            uint32_t type : 5;
            uint32_t color_id : 2;
            uint32_t color_card0_id : 4;
            uint32_t color_card1_id : 4;
            uint32_t color_card2_id : 4;
            uint32_t color_card3_id : 4;
            uint32_t color_card4_id : 4;
        } discover_cure;

        // (5) ACTION TYPE (ops_expert)
        // Destination
        // Card to discard 
        // Used for Opertions Expert Movement action
        struct {
            uint32_t type : 5;
            uint32_t target_city : 6;
            uint32_t discard_city : 6;
            uint32_t executing_player_id : 2;
            uint32_t target_player_id : 2;
            uint32_t _unused : 11;
        } ops_expert;

        // (6) ACTION TYPE (forecast)
        // Used for Forecast Action
        struct {
            uint32_t type : 5;
            uint32_t player_id : 2;
            uint32_t card_index0 : 3;
            uint32_t card_index1 : 3;
            uint32_t card_index2 : 3;
            uint32_t card_index3 : 3;
            uint32_t card_index4 : 3;
            uint32_t card_index5 : 3;
            uint32_t _unused : 7;
        } forecast;
    };

    const char* GetActionName(uint8_t type) const;
    void Print() const;
};

struct ActionList {
    const static int SIZE = 1024;
    Action actions[SIZE];
    int count = 0;

    inline Action Get(int idx) const {
        return actions[idx];
    }

    inline void Add(uint8_t type) {
        if (count < SIZE) {
            actions[count].base.type = type;
            count++;
        }
    }

    // (1) ACTION TYPE (move)
    inline void Add(uint8_t type, uint8_t city_id, uint8_t executing_player_id, uint8_t target_player_id) {
        if (count < SIZE) {
            actions[count].move.type = type;
            actions[count].move.target_city = city_id;
            actions[count].move.executing_player_id = executing_player_id;
            actions[count].move.target_player_id = target_player_id;
            count++;
        }
    }

    // (2) ACTION TYPE (treat)
    inline void Add(uint8_t type, uint8_t city_id, uint8_t player_id, ColorType color) {
        if (count < SIZE) {
            actions[count].treat.type = type;
            actions[count].treat.target_city = city_id;
            actions[count].treat.player_id = player_id;
            actions[count].treat.color_id = (uint8_t)color;
            count++;
        }
    }

    // (3) ACTION TYPE (share)
    inline void Add(uint8_t type, bool is_giving, uint8_t card_id, uint8_t player_id, uint8_t receiving_player_id) {
        if (count < SIZE) {
            actions[count].share.type = type;
            actions[count].share.target_city = card_id;
            actions[count].share.is_giving = is_giving;
            actions[count].share.player_id = player_id;
            actions[count].share.receiving_player_id = receiving_player_id;
            count++;
        }
    }

    // (4) ACTION TYPE (discover cure)
    inline void Add(uint8_t type, ColorType color, uint8_t card0, uint8_t card1, uint8_t card2, uint8_t card3, uint8_t card4) {
        if (count < SIZE) {
            actions[count].discover_cure.type = type;
            actions[count].discover_cure.color_id = (uint8_t)color;
            actions[count].discover_cure.color_card0_id = card0;
            actions[count].discover_cure.color_card1_id = card1;
            actions[count].discover_cure.color_card2_id = card2;
            actions[count].discover_cure.color_card3_id = card3;
            actions[count].discover_cure.color_card4_id = card4;
            count++;
        }
    }

    // (5) ACTION TYPE (ops expert)
    inline void Add(uint8_t type, uint8_t target_card_id, uint8_t discard_card_id, uint8_t executing_player_id, uint8_t target_player_id) {
        if (count < SIZE) {
            actions[count].ops_expert.type = type;
            actions[count].ops_expert.target_city = target_card_id;
            actions[count].ops_expert.discard_city = discard_card_id;
            actions[count].ops_expert.executing_player_id = executing_player_id;
            actions[count].ops_expert.target_player_id = target_player_id;
            count++;
        }
    }

    // (6) ACTION TYPE (forecast)
    inline void Add(uint8_t type, uint8_t player_id, uint8_t idx0, uint8_t idx1, uint8_t idx2, uint8_t idx3, uint8_t idx4, uint8_t idx5) {
        if (count < SIZE) {
            actions[count].forecast.type = type;
            actions[count].forecast.player_id = player_id;
            actions[count].forecast.card_index0 = idx0;
            actions[count].forecast.card_index1 = idx1;
            actions[count].forecast.card_index2 = idx2;
            actions[count].forecast.card_index3 = idx3;
            actions[count].forecast.card_index4 = idx4;
            actions[count].forecast.card_index5 = idx5;
            count++;
        }
    }

    // Looping
    Action* begin() { return &actions[0]; }
    Action* end() { return &actions[count]; }

    // Printing (DEBUG)
    void Print() const; 
};

#endif