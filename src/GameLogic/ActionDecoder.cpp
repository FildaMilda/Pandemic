#include "ActionDecoder.h"

Action ActionDecoder::GetActionFromIndex(int index, const GameState& state) {
    Action action;

    // Movement & Simple City-based Actions
    if (index >= ActionRanges::DRIVE_START && index < ActionRanges::DIRECT_FLIGHT_START) {
        action.base.type = ActionType::DRIVE;
        action.move.target_city = index - ActionRanges::DRIVE_START;
    }
    else if (index >= ActionRanges::DIRECT_FLIGHT_START && index < ActionRanges::CHARTER_FLIGHT_START) {
        action.base.type = ActionType::DIRECT_FLIGHT;
        action.move.target_city = index - ActionRanges::DIRECT_FLIGHT_START;
    }
    else if (index >= ActionRanges::CHARTER_FLIGHT_START && index < ActionRanges::SHUTTLE_FLIGHT_START) {
        action.base.type = ActionType::CHARTER_FLIGHT;
        action.move.target_city = index - ActionRanges::CHARTER_FLIGHT_START;
    }
    else if (index >= ActionRanges::SHUTTLE_FLIGHT_START && index < ActionRanges::BUILD_STATION) {
        action.base.type = ActionType::SHUTTLE_FLIGHT;
        action.move.target_city = index - ActionRanges::SHUTTLE_FLIGHT_START;
    }
    else if (index >= ActionRanges::BUILD_STATION && index < ActionRanges::TREAT_DISEASE_START) {
        action.base.type = ActionType::BUILD;
        action.move.target_city = index - ActionRanges::BUILD_STATION;
    }

    // Disease Treatment/Cure
    else if (index >= ActionRanges::TREAT_DISEASE_START && index < ActionRanges::SHARE_START) {
        action.base.type = ActionType::TREAT;
        action.treat.color_id = index - ActionRanges::TREAT_DISEASE_START;
        action.treat.target_city = state.players.GetLocation(state.gameFlags.GetActivePlayer());
    }
    else if (index >= ActionRanges::CURE_DISEASE_START && index < ActionRanges::CONTINGENCY_USE_START) {
        action.base.type = ActionType::CURE;
        action.discover_cure.color_id = index - ActionRanges::CURE_DISEASE_START;
        
        std::vector<uint8_t> cure_cards = state.GetBestCardsForCure(
            state.gameFlags.GetActivePlayer(),
            (ColorType)action.discover_cure.color_id
        );

        action.discover_cure.color_card0_id = cure_cards[0];
        action.discover_cure.color_card1_id = cure_cards[1];
        action.discover_cure.color_card2_id = cure_cards[2];
        action.discover_cure.color_card3_id = cure_cards[3];
        action.discover_cure.color_card4_id = cure_cards[4];
    }

    // Share (Handling two variables: player_id and city)
    else if (index >= ActionRanges::SHARE_START && index < ActionRanges::CURE_DISEASE_START) {
        action.base.type = ActionType::SHARE;
        int offset = index - ActionRanges::SHARE_START;
        action.share.receiving_player_id = offset / NUMBER_OF_CITIES;
        action.share.target_city = offset % NUMBER_OF_CITIES;
    }

    // Role Specifics
    else if (index == ActionRanges::CONTINGENCY_USE_START) {
        action.base.type = ActionType::PLANNER_USE;
    }
    else if (index >= ActionRanges::CONTINGENCY_TAKE_START && index < ActionRanges::DISPATCHER_MOVE_START) {
        action.base.type = ActionType::PLANNER_TAKE;
        action.move.target_city = (index - ActionRanges::CONTINGENCY_TAKE_START) + EventCardID::Airlift;
    }
    else if (index >= ActionRanges::DISPATCHER_MOVE_START && index < ActionRanges::OPS_EXPERT_BUILD_START) {
        action.base.type = ActionType::DISPATCHER_MOVE;
        int offset = index - ActionRanges::DISPATCHER_MOVE_START;
        action.move.target_player_id = offset / NUMBER_OF_CITIES;
        action.move.target_city = offset % NUMBER_OF_CITIES;
    }
    else if (index >= ActionRanges::OPS_EXPERT_BUILD_START && index < ActionRanges::OPS_EXPERT_MOVE_START) {
        action.base.type = ActionType::EXPERT_BUILD;
        action.move.target_city = index - ActionRanges::OPS_EXPERT_BUILD_START;
    }
    else if (index >= ActionRanges::OPS_EXPERT_MOVE_START && index < ActionRanges::GOVERMENT_GRANT_START) {
        action.base.type = ActionType::EXPERT_MOVE;
        action.ops_expert.target_city = index - ActionRanges::OPS_EXPERT_MOVE_START;
        action.ops_expert.discard_city = state.GetBestCardToDiscard(state.gameFlags.GetActivePlayer());
    }

    // Events
    else if (index >= ActionRanges::GOVERMENT_GRANT_START && index < ActionRanges::FORECAST_START) {
        action.base.type = ActionType::GOVERNMENT_GRANT;
        action.move.target_city = index - ActionRanges::GOVERMENT_GRANT_START;
        action.move.executing_player_id = state.players.GetOwnerOf(EventCardID::GovGrant);
    }
    else if (index == ActionRanges::FORECAST_START) {
        action.forecast.type = ActionType::FORECAST;
        action.forecast.player_id = state.players.GetOwnerOf(EventCardID::Forecast);;
    }
    else if (index >= ActionRanges::RESILIENT_POP_START && index < ActionRanges::ONE_QUIET_NIGHT_START) {
        action.base.type = ActionType::RESILIENT_POPULATION;
        action.move.target_city = index - ActionRanges::RESILIENT_POP_START;
        action.move.executing_player_id = state.players.GetOwnerOf(EventCardID::ResilientPopulation);;
    }
    else if (index == ActionRanges::ONE_QUIET_NIGHT_START) {
        action.base.type = ActionType::ONE_QUIET_NIGHT;
        action.move.executing_player_id = state.players.GetOwnerOf(EventCardID::OneQuietNight);;
    }
    else if (index >= ActionRanges::AIRLIFT_START && index < ActionRanges::DISCARD_CARD_START) {
        action.base.type = ActionType::AIRLIFT;
        int offset = index - ActionRanges::AIRLIFT_START;
        action.move.executing_player_id = state.players.GetOwnerOf(EventCardID::Airlift);;
        action.move.target_player_id = offset / NUMBER_OF_CITIES;
        action.move.target_city = offset % NUMBER_OF_CITIES;
    }

    // Cleanup / System
    else if (index >= ActionRanges::DISCARD_CARD_START && index < ActionRanges::REMOVE_STATION_START) {
        action.base.type = ActionType::DISCARD_CARD;
        action.move.target_city = index - ActionRanges::DISCARD_CARD_START;
        action.move.executing_player_id = state.gameFlags.GetActivePlayer();
    }
    else if (index >= ActionRanges::REMOVE_STATION_START && index < ActionRanges::END_TURN_START) {
        action.base.type = ActionType::REMOVE_STATION;
        action.move.target_city = index - ActionRanges::REMOVE_STATION_START;
        action.move.executing_player_id = state.gameFlags.GetActivePlayer();
    }
    else if (index == ActionRanges::END_TURN_START) {
        action.base.type = ActionType::END_TURN;
    }

    // Safety Catch
    else {
        action.raw_data = std::numeric_limits<uint32_t>::max();
    }

    return action;
}

int ActionDecoder::GetIndexFromAction(const Action& action) {
    switch (action.base.type) {

        // Movement actions:
    case ActionType::DRIVE:
        return ActionRanges::DRIVE_START + action.move.target_city;

    case ActionType::DIRECT_FLIGHT:
        return ActionRanges::DIRECT_FLIGHT_START + action.move.target_city;

    case ActionType::CHARTER_FLIGHT:
        return ActionRanges::CHARTER_FLIGHT_START + action.move.target_city;

    case ActionType::SHUTTLE_FLIGHT:
        return ActionRanges::SHUTTLE_FLIGHT_START + action.move.target_city;

        // Other actions:
    case ActionType::BUILD:
        return ActionRanges::BUILD_STATION + action.move.target_city;
    case ActionType::TREAT:
        return ActionRanges::TREAT_DISEASE_START + action.treat.color_id;
    case ActionType::SHARE:
        return ActionRanges::SHARE_START + (action.share.receiving_player_id * NUMBER_OF_CITIES) + action.share.target_city;
    case ActionType::CURE:
        return ActionRanges::CURE_DISEASE_START + action.discover_cure.color_id;

        // Role actions:
    case ActionType::PLANNER_USE:
        return ActionRanges::CONTINGENCY_USE_START;
    case ActionType::PLANNER_TAKE:
        return ActionRanges::CONTINGENCY_TAKE_START + action.move.target_city - EventCardID::Airlift;

    case ActionType::DISPATCHER_MOVE:
        return ActionRanges::DISPATCHER_MOVE_START + (action.move.target_player_id * NUMBER_OF_CITIES) + action.move.target_city;
    case ActionType::DISPATCHER_MOVE_AS:
        return ActionRanges::DISPATCHER_MOVE_START + (action.move.target_player_id * NUMBER_OF_CITIES) + action.move.target_city;

    case ActionType::EXPERT_BUILD:
        return ActionRanges::OPS_EXPERT_BUILD_START + action.move.target_city;
    case ActionType::EXPERT_MOVE:
        return ActionRanges::OPS_EXPERT_MOVE_START + action.ops_expert.target_city;

        // Event actions:
    case ActionType::GOVERNMENT_GRANT:
        return ActionRanges::GOVERMENT_GRANT_START + action.move.target_city;
    case ActionType::FORECAST:
        return ActionRanges::FORECAST_START;
    case ActionType::RESILIENT_POPULATION:
        return ActionRanges::RESILIENT_POP_START + action.move.target_city;
    case ActionType::ONE_QUIET_NIGHT:
        return ActionRanges::ONE_QUIET_NIGHT_START;
    case ActionType::AIRLIFT:
        return ActionRanges::AIRLIFT_START + (action.move.target_player_id * NUMBER_OF_CITIES) + action.move.target_city;

    case ActionType::DISCARD_CARD:
        return ActionRanges::DISCARD_CARD_START + action.move.target_city;
    case ActionType::REMOVE_STATION:
        return ActionRanges::REMOVE_STATION_START + action.move.target_city;
    case ActionType::END_TURN:
        return ActionRanges::END_TURN_START;

    default:
        return -1; // Should not happen for legal actions
    }
}