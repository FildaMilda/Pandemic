#include "Action.h"

void Action::Print() const 
{
    std::cout << GetActionName(base.type) << " | ";

    switch (base.type) {
        // (1) MOVE LENS
    case DRIVE: case DIRECT_FLIGHT: case CHARTER_FLIGHT:
    case SHUTTLE_FLIGHT: case BUILD: case EXPERT_BUILD:
    case DISPATCHER_MOVE: case DISPATCHER_MOVE_AS:
    case AIRLIFT: case DISCARD_CARD: case GOVERNMENT_GRANT:
    case RESILIENT_POPULATION:
        std::cout << "Player (origin): " << (int)move.executing_player_id
            << ", Player (target): " << (int)move.target_player_id
            << ", Target City/Card ID: " << (int)move.target_city;
        break;

        // (2) TREAT LENS
    case TREAT:
        std::cout << "Player: " << (int)treat.player_id
            << ", City: " << (int)treat.target_city
            << ", Color: " << (int)treat.color_id;
        break;

        // (3) SHARE LENS
    case SHARE:
        std::cout << "Player: " << (int)share.player_id
            << (share.is_giving ? " GIVES card " : " TAKES card ")
            << (int)share.target_city
            << (share.is_giving ? " TO " : " FROM ")
            << "Player: " << (int)share.receiving_player_id;
        break;

        // (4) DISCOVER CURE LENS
    case CURE:
        std::cout << "Color: " << (int)discover_cure.color_id
            << ", Cards Used: [" << (int)discover_cure.color_card0_id << ", "
            << (int)discover_cure.color_card1_id << ", "
            << (int)discover_cure.color_card2_id << ", "
            << (int)discover_cure.color_card3_id << ", "
            << (int)discover_cure.color_card4_id << "]";
        break;

        // (5) OPS EXPERT LENS
    case EXPERT_MOVE:
        std::cout << "Player: " << (int)ops_expert.executing_player_id
            << ", Target: " << (int)ops_expert.target_city
            << ", Discarding Card: " << (int)ops_expert.discard_city;
        break;

        // (6) FORECAST LENS
    case FORECAST:
        std::cout << "New Order Indices: ["
            << (int)forecast.card_index0 << ", "
            << (int)forecast.card_index1 << ", "
            << (int)forecast.card_index2 << ", "
            << (int)forecast.card_index3 << ", "
            << (int)forecast.card_index4 << ", "
            << (int)forecast.card_index5 << "]";
        break;
    }
    std::cout << "\n";
}

const char* Action::GetActionName(uint8_t type) const
{
    switch (type) {
    case DRIVE: return "DRIVE";
    case DIRECT_FLIGHT: return "DIRECT_FLIGHT";
    case CHARTER_FLIGHT: return "CHARTER_FLIGHT";
    case SHUTTLE_FLIGHT: return "SHUTTLE_FLIGHT";
    case BUILD: return "BUILD";
    case TREAT: return "TREAT";
    case SHARE: return "SHARE";
    case CURE: return "CURE";
    case PLANNER_TAKE: return "PLANNER_TAKE";
    case DISPATCHER_MOVE: return "DISPATCHER_MOVE";
    case DISPATCHER_MOVE_AS: return "DISPATCHER_MOVE_AS";
    case EXPERT_BUILD: return "EXPERT_BUILD";
    case EXPERT_MOVE: return "EXPERT_MOVE";
    case GOVERNMENT_GRANT: return "GOVERNMENT_GRANT";
    case FORECAST: return "FORECAST";
    case RESILIENT_POPULATION: return "RESILIENT_POPULATION";
    case ONE_QUIET_NIGHT: return "ONE_QUIET_NIGHT";
    case AIRLIFT: return "AIRLIFT";
    case DISCARD_CARD: return "DISCARD_CARD";
    case REMOVE_STATION: return "REMOVE_STATION";
    case END_TURN: return "END_TURN";
    default: return "UNKNOWN_ACTION";
    }
}

void ActionList::Print() const
{
    std::cout << "--- Action List (" << count << " possible moves) ---\n";

    for (int i = 0; i < count; i++) {
        const Action& a = actions[i];
        a.Print();
    }
    std::cout << "--------------------------------------\n";
}

std::string GetActionString(const Action& action)
{
    ActionType type = static_cast<ActionType>(action.base.type);

    switch (type) {
        // --- MOVE ACTIONS ---
    case DRIVE:
        return "Drive: Player " + std::to_string(action.move.executing_player_id) +
            " to " + CardRegistry::GetName(action.move.target_city);
    case DIRECT_FLIGHT:
        return "Direct Flight: Player " + std::to_string(action.move.executing_player_id) +
            " to " + CardRegistry::GetName(action.move.target_city);
    case CHARTER_FLIGHT:
        return "Charter Flight: Player " + std::to_string(action.move.executing_player_id) +
            " to " + CardRegistry::GetName(action.move.target_city);
    case SHUTTLE_FLIGHT:
        return "Shuttle Flight: Player " + std::to_string(action.move.executing_player_id) +
            " to " + CardRegistry::GetName(action.move.target_city);

        // --- STANDARD ACTIONS ---
    case BUILD:
        return "Build Research Station: Player " + std::to_string(action.move.executing_player_id) +
            " at " + CardRegistry::GetName(action.move.target_city);
    case TREAT:
        return "Treat Disease: Player " + std::to_string(action.treat.player_id) +
            " at " + CardRegistry::GetName(action.treat.target_city) +
            " (Color ID: " + std::to_string(action.treat.color_id) + ")";
    case SHARE:
        return std::string("Share Knowledge: Player ") + std::to_string(action.share.player_id) +
            (action.share.is_giving ? " gives " : " takes ") +
            CardRegistry::GetName(action.share.target_city) +
            (action.share.is_giving ? " to Player " : " from Player ") +
            std::to_string(action.share.receiving_player_id);
    case CURE:
        return "Discover Cure: Color " + std::to_string(action.discover_cure.color_id) +
            " using [" +
            CardRegistry::GetName(action.discover_cure.color_card0_id) + ", " +
            CardRegistry::GetName(action.discover_cure.color_card1_id) + ", " +
            CardRegistry::GetName(action.discover_cure.color_card2_id) + ", " +
            CardRegistry::GetName(action.discover_cure.color_card3_id) + ", " +
            CardRegistry::GetName(action.discover_cure.color_card4_id) + "]";

        // --- ROLE SPECIFIC ACTIONS ---
    case DISPATCHER_MOVE:
        return "Dispatcher Move: Dispatcher moves Player " + std::to_string(action.move.target_player_id) +
            " to " + CardRegistry::GetName(action.move.target_city);
    case DISPATCHER_MOVE_AS:
        return "Dispatcher Move As: Dispatcher moves Player " + std::to_string(action.move.target_player_id) +
            " to " + CardRegistry::GetName(action.move.target_city) + " as a standard move";
    case EXPERT_BUILD:
        return "Ops Expert Build: Player " + std::to_string(action.ops_expert.executing_player_id) +
            " at " + CardRegistry::GetName(action.ops_expert.target_city);
    case EXPERT_MOVE:
        return "Ops Expert Move: Player " + std::to_string(action.ops_expert.executing_player_id) +
            " to " + CardRegistry::GetName(action.ops_expert.target_city) +
            " discarding " + CardRegistry::GetName(action.ops_expert.discard_city);
    case PLANNER_TAKE:
        return "Planner Take: Retrieve " + CardRegistry::GetName(action.move.target_city) + " from discard";
    case PLANNER_USE:
        return "Planner Use: Use stored Event Card";

        // --- EVENT CARDS ---
    case GOVERNMENT_GRANT:
        return "Event (Govt Grant): Build Station at " + CardRegistry::GetName(action.move.target_city);
    case AIRLIFT:
        return "Event (Airlift): Move Player " + std::to_string(action.move.target_player_id) +
            " to " + CardRegistry::GetName(action.move.target_city);
    case RESILIENT_POPULATION:
        return "Event (Resilient Pop): Remove " + CardRegistry::GetName(action.move.target_city);
    case ONE_QUIET_NIGHT:
        return "Event (One Quiet Night): Skip next infection step";
    case FORECAST:
        return "Event (Forecast): Player " + std::to_string(action.forecast.player_id) +
            " rearranged top cards to [" +
            CardRegistry::GetName(action.forecast.card_index0) + ", " +
            CardRegistry::GetName(action.forecast.card_index1) + ", " +
            CardRegistry::GetName(action.forecast.card_index2) + ", " +
            CardRegistry::GetName(action.forecast.card_index3) + ", " +
            CardRegistry::GetName(action.forecast.card_index4) + ", " +
            CardRegistry::GetName(action.forecast.card_index5) + "]";

        // --- GAME SYSTEM ACTIONS ---
    case DISCARD_CARD:
        return "Discard Card: Player " + std::to_string(action.move.executing_player_id) +
            " discards " + CardRegistry::GetName(action.move.target_city);
    case REMOVE_STATION:
        return "Remove Station: Remove from " + CardRegistry::GetName(action.move.target_city);
    case END_TURN:
        return "End Turn";

    default:
        return "Unknown Action (Type ID: " + std::to_string(static_cast<int>(type)) + ")";
    }
}
