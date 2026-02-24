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
        std::cout << "Player (origin): " << (int)move.player_id
            << ", Player (target): " << (int)move.player_id2
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
        std::cout << "Player: " << (int)ops_expert.player_id
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
