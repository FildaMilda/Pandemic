#include "ActionDecoder.h"

Action ActionDecoder::GetActionFromIndex(int index, const GameState& state) {
    Action action;

    // 1. DRIVE / FERRY
    if (index >= ActionRanges::DRIVE_START && index < ActionRanges::DIRECT_FLIGHT_START) {
        action.move.type = ActionType::DRIVE;
        action.move.target_city = index - ActionRanges::DRIVE_START;
    }
    // 2. DIRECT FLIGHT
    else if (index >= ActionRanges::DIRECT_FLIGHT_START && index < ActionRanges::CHARTER_FLIGHT_START) {
        action.move.type = ActionType::DIRECT_FLIGHT;
        action.move.target_city = index - ActionRanges::DIRECT_FLIGHT_START;
    }
    // 3. CHARTER FLIGHT
    else if (index >= ActionRanges::CHARTER_FLIGHT_START && index < ActionRanges::SHUTTLE_FLIGHT_START) {
        action.move.type = ActionType::CHARTER_FLIGHT;
        action.move.target_city = index - ActionRanges::CHARTER_FLIGHT_START;
    }
    // 4. SHUTTLE FLIGHT
    else if (index >= ActionRanges::SHUTTLE_FLIGHT_START && index < ActionRanges::BUILD_STATION) {
        action.move.type = ActionType::SHUTTLE_FLIGHT;
        action.move.target_city = index - ActionRanges::SHUTTLE_FLIGHT_START;
    }

    return action;
}

int ActionDecoder::GetIndexFromAction(const Action& action) {
    switch (action.base.type) {
    case ActionType::DRIVE:
        return ActionRanges::DRIVE_START + action.move.target_city;

    case ActionType::DIRECT_FLIGHT:
        return ActionRanges::DIRECT_FLIGHT_START + action.move.target_city;

    case ActionType::CHARTER_FLIGHT:
        return ActionRanges::CHARTER_FLIGHT_START + action.move.target_city;

    case ActionType::SHUTTLE_FLIGHT:
        return ActionRanges::SHUTTLE_FLIGHT_START + action.move.target_city;

    default:
        return -1; // Should not happen for legal actions
    }
}