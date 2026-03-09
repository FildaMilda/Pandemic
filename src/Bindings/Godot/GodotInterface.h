#ifndef PANDEMIC_GAME_H
#define PANDEMIC_GAME_H

#include <godot_cpp/classes/node.hpp>
#include <godot_cpp/variant/dictionary.hpp>
#include <godot_cpp/variant/array.hpp>
#include <random>
#include <cstdint>
#include "Game.h"

namespace godot {

    class PandemicGame : public godot::Node {
        GDCLASS(PandemicGame, godot::Node)

    private:
        GameState game;
        std::mt19937 rng;

    protected:
        static void _bind_methods();

    public:
        PandemicGame();
        ~PandemicGame();

        void setup_game(int seed);

        TypedArray<int> get_possible_actions();
        void execute_action(int64_t p_raw_data);

        int get_game_state();
        int get_player_location(int player_id);
        Array get_player_hand(int player_id);
        int get_city_infection(int city_id, int color_id);
        bool has_research_station(int city_id);
        int get_outbreak_count();
        int get_actions_left();
        int get_player_count();
        int get_player_role(int player_id);
        int get_current_player();

        int get_card_type(int card_id);
        String get_card_name(int card_id);
        int get_card_color(int card_id);

        Array get_city_neighbors(int city_id);
    };

}

#endif