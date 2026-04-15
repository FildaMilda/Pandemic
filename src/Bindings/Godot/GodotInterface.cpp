#include "GodotInterface.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>
#include <bit>

using namespace godot;

void PandemicGame::_bind_methods() {
    // --- Signals ---
    ADD_SIGNAL(MethodInfo("game_updated"));

    // --- Core Game Flow ---
    ClassDB::bind_method(D_METHOD("setup_game", "difficulty", "players", "seed"), &PandemicGame::setup_game);
    ClassDB::bind_method(D_METHOD("get_possible_actions"), &PandemicGame::get_possible_actions);
    ClassDB::bind_method(D_METHOD("execute_action", "raw_data"), &PandemicGame::execute_action);

    // --- Getters (State & Flags) ---
    ClassDB::bind_method(D_METHOD("get_game_state"), &PandemicGame::get_game_state);
    ClassDB::bind_method(D_METHOD("get_outbreak_count"), &PandemicGame::get_outbreak_count);
    ClassDB::bind_method(D_METHOD("get_actions_left"), &PandemicGame::get_actions_left);
    ClassDB::bind_method(D_METHOD("get_player_discard_pile"), &PandemicGame::get_player_discard_pile);
    ClassDB::bind_method(D_METHOD("get_infection_discard_pile"), &PandemicGame::get_infection_discard_pile);
    ClassDB::bind_method(D_METHOD("get_infection_rate_amount"), &PandemicGame::get_infection_rate_amount);

    // --- Player Data ---
    ClassDB::bind_method(D_METHOD("get_player_location", "player_id"), &PandemicGame::get_player_location);
    ClassDB::bind_method(D_METHOD("get_player_hand", "player_id"), &PandemicGame::get_player_hand);
    ClassDB::bind_method(D_METHOD("get_player_count"), &PandemicGame::get_player_count);
    ClassDB::bind_method(D_METHOD("get_player_role", "player_id"), &PandemicGame::get_player_role);
    ClassDB::bind_method(D_METHOD("get_current_player"), &PandemicGame::get_current_player);
    ClassDB::bind_method(D_METHOD("is_planner_empty"), &PandemicGame::is_planner_empty);
    ClassDB::bind_method(D_METHOD("get_planner_slot"), &PandemicGame::get_planner_slot);

    // --- Map Data ---
    ClassDB::bind_method(D_METHOD("get_city_infection", "city_id", "color_id"), &PandemicGame::get_city_infection);
    ClassDB::bind_method(D_METHOD("has_research_station", "city_id"), &PandemicGame::has_research_station);
    ClassDB::bind_method(D_METHOD("get_stations"), &PandemicGame::get_stations);
    ClassDB::bind_method(D_METHOD("get_hotspots"), &PandemicGame::get_hotspots);
    ClassDB::bind_method(D_METHOD("get_forecast_cards"), &PandemicGame::get_forecast_cards);
    ClassDB::bind_method(D_METHOD("do_forecast"), &PandemicGame::do_forecast);

    ClassDB::bind_method(D_METHOD("get_card_type", "card_id"), &PandemicGame::get_card_type);
    ClassDB::bind_method(D_METHOD("get_card_name", "card_id"), &PandemicGame::get_card_name);
    ClassDB::bind_method(D_METHOD("get_card_color", "card_id"), &PandemicGame::get_card_color);
    ClassDB::bind_method(D_METHOD("get_event_action_id", "card_id"), &PandemicGame::get_event_action_id);

    ClassDB::bind_method(D_METHOD("get_city_neighbors", "city_id"), &PandemicGame::get_city_neighbors);

    ClassDB::bind_method(D_METHOD("is_game_over"), &PandemicGame::is_game_over);
    ClassDB::bind_method(D_METHOD("get_mcts_macro_action", "iterations"), &PandemicGame::get_mcts_macro_action);
    ClassDB::bind_method(D_METHOD("clone"), &PandemicGame::clone);
    ClassDB::bind_method(D_METHOD("test"), &PandemicGame::test);
}

PandemicGame::PandemicGame() {}
PandemicGame::~PandemicGame() {}

void PandemicGame::setup_game(int diff, int player_count, int seed) {
    rng.seed(seed);
    game.Setup((Difficulty)diff, player_count, &rng);
    emit_signal("game_updated");
}

TypedArray<int> godot::PandemicGame::get_possible_actions()
{
    ActionList list;
    list.count = 0;
    game.GetPossibleActions(list);

    TypedArray<int> godot_list;
    for (int i = 0; i < list.count; ++i) {
        godot_list.push_back((int64_t)list.actions[i].raw_data);
    }
    return godot_list;
}

Array godot::PandemicGame::get_player_discard_pile()
{
    Array cards;
    for (const auto card : game.decks.player_deck.GetDiscardPile()) {
        cards.push_back(card);
    }
    return cards;
}

Array godot::PandemicGame::get_infection_discard_pile()
{
    Array cards;
    for (const auto card : game.decks.infection_deck.GetDiscardPile()) {
        cards.push_back(card);
    }
    return cards;
}

godot::Dictionary godot::PandemicGame::execute_action(int64_t p_raw_data)
{
    Action action;
    action.raw_data = static_cast<uint32_t>(p_raw_data);
    DrawnCards cards;
    game.Execute(action, &cards);

    // Pack into a Godot Dictionary
    godot::Dictionary result;

    // Add the new flag
    result["turn_ended"] = cards.turnEnded;

    // Build and add the arrays
    godot::Array gd_player_cards;
    for (uint8_t card : cards.drawnPlayerCards) gd_player_cards.push_back(card);
    result["drawn_player_cards"] = gd_player_cards;

    godot::Array gd_infection_cards;
    for (uint8_t card : cards.drawnInfectionCards) gd_infection_cards.push_back(card);
    result["drawn_infection_cards"] = gd_infection_cards;

    return result;
}

// --- Getter Implementations ---

int PandemicGame::get_game_state() {
    return (int)game.currentState;
}

int PandemicGame::get_player_location(int player_id) {
    return (int)game.players.GetLocation(player_id);
}

Array godot::PandemicGame::get_player_hand(int player_id)
{
    uint64_t temp_hand = game.players.hands[player_id];
    Array hand_array;

    while (temp_hand > 0) {
        uint8_t cardId = std::countr_zero(temp_hand);
        hand_array.append(cardId);
        temp_hand &= (temp_hand - 1);
    }

    return hand_array;
}

int PandemicGame::get_city_infection(int city_id, int color_id) {
    return (int)game.cityState.GetCubeCount(city_id, (ColorType)color_id);
}

bool godot::PandemicGame::has_research_station(int city_id)
{
    return game.cityState.HasStation(city_id);
}

int godot::PandemicGame::get_outbreak_count()
{
    return (int)game.gameFlags.GetOutbreaks();
}

int godot::PandemicGame::get_infection_rate_amount()
{
    return (int)game.gameFlags.GetInfectionRateAmount();
}

int PandemicGame::get_actions_left() {
    return (int)game.gameFlags.GetActionsRemaining();
}

int godot::PandemicGame::get_player_count()
{
    return (int)game.players.count;
}

int godot::PandemicGame::get_player_role(int player_id)
{
    return (int)game.players.GetRole(player_id);
}

int godot::PandemicGame::get_current_player()
{
    return (int)game.gameFlags.GetActivePlayer();
}

Array godot::PandemicGame::get_stations()
{
    Array stations;
    uint64_t mask = game.cityState.GetStationMask();

    while (mask != 0)
    {
        int index = std::countr_zero(mask);
        stations.append(index);
        mask &= (mask - 1);
    }

    return stations;
}

Array godot::PandemicGame::get_hotspots()
{
    Array hotspots;
    uint64_t mask = game.cityState.GetHotspotMask();

    while (mask != 0)
    {
        int index = std::countr_zero(mask);
        hotspots.append(index);
        mask &= (mask - 1);
    }

    return hotspots;
}

bool godot::PandemicGame::is_planner_empty()
{
    return game.gameFlags.IsContingencyPlannerSlotEmpty();
}

int godot::PandemicGame::get_planner_slot()
{
    return game.gameFlags.GetContingencyPlannerSlot();
}

void godot::PandemicGame::do_forecast(int card0, int card1, int card2, int card3, int card4, int card5)
{
    uint8_t owner = game.players.GetOwnerOf(EventCardID::Forecast);
    game.DoForecast(owner, card0, card1, card2, card3, card4, card5);
}

int godot::PandemicGame::get_card_type(int card_id)
{
    return (int)CardRegistry::GetType(card_id);
}

String godot::PandemicGame::get_card_name(int card_id)
{
    return String(CardRegistry::GetName(card_id).c_str());
}

int godot::PandemicGame::get_card_color(int card_id)
{
    return (int)CardRegistry::GetColor(card_id);
}

int godot::PandemicGame::get_event_action_id(int card_id)
{
    if (card_id == EventCardID::Airlift) return ActionType::AIRLIFT;
    if (card_id == EventCardID::GovGrant) return ActionType::GOVERNMENT_GRANT;
    if (card_id == EventCardID::ResilientPopulation) return ActionType::RESILIENT_POPULATION;
    if (card_id == EventCardID::Forecast) return ActionType::FORECAST;
    if (card_id == EventCardID::OneQuietNight) return ActionType::ONE_QUIET_NIGHT;
    else godot::print_error("Wrong card_id {}", card_id);
}

Array godot::PandemicGame::get_forecast_cards()
{
    Array forecast_array;
    uint8_t out_cards[6];

    // PeekForecastCards returns the number of valid cards it pulled from the Draw Pile
    int count = game.decks.infection_deck.PeekForecastCards(out_cards);

    // If the draw deck is empty, return the empty array
    if (count == 0) {
        return forecast_array;
    }

    // Append the valid cards to the Godot Array
    for (int i = 0; i < count; i++) {
        forecast_array.append(out_cards[i]);
    }

    return forecast_array;
}

Array godot::PandemicGame::get_city_neighbors(int city_id)
{
    Array neighbors;

    if (city_id < 0 || city_id >= NUMBER_OF_CITIES) {
        return neighbors;
    }

    const uint8_t* neighbor_ptr = MapData::GetNeighbors(static_cast<uint8_t>(city_id));

    // Iterate through the fixed size of 8
    for (int i = 0; i < 8; ++i) {
        uint8_t neighbor_id = neighbor_ptr[i];

        // 255 is your 'null' marker. Stop adding if we hit it.
        if (neighbor_id == 255) {
            break;
        }

        neighbors.append(neighbor_id);
    }

    return neighbors;
}

Array godot::PandemicGame::get_mcts_macro_action(int iterations)
{
    Array actions;
    Turn bestMacro = mcts.Search(game, iterations);

    for (int i = 0; i < bestMacro.count; i++) {
        actions.push_back((int64_t)bestMacro.actions[i].raw_data);
    }

    return actions;
}

bool godot::PandemicGame::is_game_over()
{
    return game.currentState != State::InProgress;
}

PandemicGame* godot::PandemicGame::clone()
{
    PandemicGame* cloned_node = memnew(PandemicGame);
    cloned_node->game = this->game;
    cloned_node->rng = this->rng;
    return cloned_node;
}

void godot::PandemicGame::test()
{
    godot::print_line("Test");
}

void initialize_pandemic_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

    CardRegistry::Initialize();

    ClassDB::register_class<PandemicGame>();
}

void uninitialize_pandemic_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }
}

extern "C" {
    GDExtensionBool GDE_EXPORT pandemic_library_init(GDExtensionInterfaceGetProcAddress p_get_proc_address, const GDExtensionClassLibraryPtr p_library, GDExtensionInitialization* r_initialization) {
        godot::GDExtensionBinding::InitObject init_obj(p_get_proc_address, p_library, r_initialization);

        init_obj.register_initializer(initialize_pandemic_module);
        init_obj.register_terminator(uninitialize_pandemic_module);
        init_obj.set_minimum_library_initialization_level(MODULE_INITIALIZATION_LEVEL_SCENE);

        return init_obj.init();
    }
}