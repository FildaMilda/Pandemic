#include "GodotInterface.h"
#include <godot_cpp/core/class_db.hpp>
#include <godot_cpp/core/defs.hpp>
#include <godot_cpp/godot.hpp>

using namespace godot;

void PandemicGame::_bind_methods() {
    // --- Signals ---
    ADD_SIGNAL(MethodInfo("game_updated"));

    // --- Core Game Flow ---
    ClassDB::bind_method(D_METHOD("setup_game", "seed"), &PandemicGame::setup_game);
    ClassDB::bind_method(D_METHOD("get_possible_actions"), &PandemicGame::get_possible_actions);
    ClassDB::bind_method(D_METHOD("execute_action", "raw_data"), &PandemicGame::execute_action);

    // --- Getters (State & Flags) ---
    ClassDB::bind_method(D_METHOD("get_game_state"), &PandemicGame::get_game_state);
    ClassDB::bind_method(D_METHOD("get_outbreak_count"), &PandemicGame::get_outbreak_count);
    ClassDB::bind_method(D_METHOD("get_actions_left"), &PandemicGame::get_actions_left);

    // --- Player Data ---
    ClassDB::bind_method(D_METHOD("get_player_location", "player_id"), &PandemicGame::get_player_location);
    ClassDB::bind_method(D_METHOD("get_player_hand", "player_id"), &PandemicGame::get_player_hand);

    // --- Map Data ---
    ClassDB::bind_method(D_METHOD("get_city_infection", "city_id", "color_id"), &PandemicGame::get_city_infection);
    ClassDB::bind_method(D_METHOD("has_research_station", "city_id"), &PandemicGame::has_research_station);
}

PandemicGame::PandemicGame() {}
PandemicGame::~PandemicGame() {}

void PandemicGame::setup_game(int seed) {
    rng.seed(seed);
    game.Setup(Difficulty::INTRO, 4, &rng); //TODO: add diff and count args
    emit_signal("game_updated");
}

TypedArray<int> godot::PandemicGame::get_possible_actions()
{
    ActionList list;
    game.GetPossibleActions(list);

    TypedArray<int> godot_list;
    for (int i = 0; i < list.count; ++i) {
        godot_list.push_back((int64_t)list.actions[i].raw_data);
    }
    return godot_list;
}

void godot::PandemicGame::execute_action(int64_t p_raw_data)
{
    Action action;
    action.raw_data = static_cast<uint32_t>(p_raw_data);
    game.Execute(action);
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
    return Array();
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

int PandemicGame::get_actions_left() {
    return (int)game.gameFlags.GetActionsRemaining();
}

void initialize_pandemic_module(ModuleInitializationLevel p_level) {
    if (p_level != MODULE_INITIALIZATION_LEVEL_SCENE) {
        return;
    }

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