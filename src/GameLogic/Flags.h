#ifndef FLAGS_H
#define FLAGS_H

#include "Globals.h"

#include <cstdint>
#include <iostream>

struct GameFlags {
    // Status Counters
    uint32_t outbreak_counter : 4;       // Range: 0-8
    uint32_t infection_rate_idx : 3;     // Range: 0-6  

    // Turn State
    uint32_t active_player_idx : 2;      // Range: 0-3
    uint32_t actions_remaining : 3;      // Range: 0-4

    // Disease Status (4 bits each)
    // Bit 0=Blue, 1=Yellow, 2=Black, 3=Red
    uint32_t cured_bits : 4;
    uint32_t eradicated_bits : 4;

    // Special Event Flags
    uint32_t one_quiet_night : 1;

    // Special Roles Flags
    uint32_t contingency_planner_slot : 3;
    uint32_t operations_expert_movement_used : 1;
    uint32_t is_quarantine_specialist_in_game : 1;
    uint32_t quarantine_specialist_id : 2;
    uint32_t is_medic_in_game : 1;
    uint32_t medic_id : 2;

    uint32_t _unused : 1;

    void Init() {
        outbreak_counter = 0;
        infection_rate_idx = 0;

        active_player_idx = 0;
        actions_remaining = 4;

        cured_bits = 0;
        eradicated_bits = 0;

        one_quiet_night = 0;

        contingency_planner_slot = 7; // MAX = EMPTY
        operations_expert_movement_used = false;
        is_quarantine_specialist_in_game = false;
        quarantine_specialist_id = 0;
        is_medic_in_game = 0;
        medic_id = 0;
    }

    inline void SetContingencyPlannerSlot(uint8_t cardId) {
        contingency_planner_slot = cardId - (uint8_t)EventCardID::Airlift;
    }
    inline uint8_t GetContingencyPlannerSlot() const {
        return (uint8_t)EventCardID::Airlift + contingency_planner_slot;
    }
    inline bool IsContingencyPlannerSlotEmpty() const {
        return contingency_planner_slot == 7;
    }
    inline void EmptyContingencyPlannerSlot() {
        contingency_planner_slot = 7;
    }

    inline void SetOperationsExpertMovFlag(bool isUsed) {
        operations_expert_movement_used = isUsed;
    }
    inline bool HasOpsExpertUsedFlight() const {
        return operations_expert_movement_used;
    }

    inline void SetQuarantineFlag(bool isInGame) {
        is_quarantine_specialist_in_game = isInGame;
    }
    inline bool IsGuarantineSpecialistInGame() const {
        return is_quarantine_specialist_in_game;
    }
    inline void SetQuarantineSpecialistID(uint8_t idx) {
        quarantine_specialist_id = idx;
    }
    inline uint8_t GetQuarantineSpecialistID() const {
        return quarantine_specialist_id;
    }

    inline void SetMedicFlag(bool isInGame) {
        is_medic_in_game = isInGame;
    }
    inline bool IsMedicInGame() const {
        return is_medic_in_game;
    }
    inline void SetMedicID(uint8_t idx) {
        medic_id = idx;
    }
    inline uint8_t GetMedicID() const {
        return medic_id;
    }

    inline int GetOutbreaks() const { return outbreak_counter; }
    inline void IncOutbreaks() {
        if (outbreak_counter < OUTBREAK_MARKER_MAX) outbreak_counter++;
    }

    inline int GetInfectionRateIndex() const { return infection_rate_idx; }
    inline void IncInfectionRate() {
        if (infection_rate_idx < 6) infection_rate_idx++;
    }
    // Returns actual # of cards to draw (2, 2, 2, 3, 3, 4, 4)
    inline int GetInfectionRateAmount() const {
        // Hardcoded lookup for speed
        if (infection_rate_idx < 3) return 2;
        if (infection_rate_idx < 5) return 3;
        return 4;
    }

    inline int GetActivePlayer() const { return active_player_idx; }
    inline void NextPlayer() {
        active_player_idx = (active_player_idx + 1) % 4;
        actions_remaining = 4; // Reset actions
    }

    inline int GetActionsRemaining() const { return actions_remaining; }
    inline void UseAction() {
        if (actions_remaining > 0) actions_remaining--;
    }
    inline void SetActionsRemaining(uint8_t value) {
        actions_remaining = value;
    }

    inline bool IsCured(ColorType colorIdx) const {
        return (cured_bits >> (int)colorIdx) & 1;
    }
    inline void SetCured(ColorType colorIdx) {
        cured_bits |= (1 << (int)colorIdx);
    }
    inline bool IsAllCured() const {
        return IsCured(ColorType::RED) && IsCured(ColorType::BLACK) && IsCured(ColorType::BLUE) && IsCured(ColorType::YELLOW);
    }
    inline uint8_t GetCuredCount() const {
        return std::popcount(cured_bits);
    }

    inline bool IsEradicated(ColorType colorIdx) const {
        return (eradicated_bits >> (int)colorIdx) & 1;
    }
    inline void SetEradicated(ColorType colorIdx) {
        eradicated_bits |= (1 << (int)colorIdx);
    }
    inline bool AreAllCured() const {
        return cured_bits == 0b1111; // Check if all 4 bits are 1
    }

    inline bool IsQuietNight() const { return one_quiet_night; }
    inline void SetQuietNight(bool active) { one_quiet_night = active; }
};

#endif