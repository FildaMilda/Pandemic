extends Node

# Preload the main scene at the very start of the application
var preloaded_main_scene: PackedScene = preload("res://scenes/main.tscn")

var game_difficulty: int = 1
var game_players: int = 4
var game_seed: int = 42

enum CityColor {BLUE, YELLOW, BLACK, RED}
enum ActionType {
	DRIVE = 0,          
	DIRECT_FLIGHT = 1,  
	CHARTER_FLIGHT = 2, 
	SHUTTLE_FLIGHT = 3, 
	BUILD = 4,          
	TREAT = 5,          
	SHARE = 6,          
	CURE = 7,

	PLANNER_TAKE = 8,           
	DISPATCHER_MOVE = 9,       
	DISPATCHER_MOVE_AS = 10,    
	EXPERT_BUILD = 11,         
	EXPERT_MOVE = 12,          

	GOVERNMENT_GRANT = 13,
	FORECAST = 14,
	RESILIENT_POPULATION = 15,
	ONE_QUIET_NIGHT = 16,
	AIRLIFT = 17,

	DISCARD_CARD = 18,
	REMOVE_STATION = 19,

	END_TURN = 20
}

func is_movement_action(type : int) -> bool:
	if type in [ActionType.DRIVE, ActionType.DIRECT_FLIGHT, ActionType.CHARTER_FLIGHT, ActionType.SHUTTLE_FLIGHT]:
		return true
	return false

func get_event_description(event_action_id : int) -> String:
	match event_action_id:
		ActionType.GOVERNMENT_GRANT:
			return "Add 1 research station to any city (no city card needed)"
		ActionType.FORECAST:
			return "Draw, Look at, and rearange the top 6 cards of the infection deck. Put them back on top"
		ActionType.AIRLIFT:
			return "Move any 1 pawn to any city, get permission before moving another player's pawn"
		ActionType.RESILIENT_POPULATION:
			return "Remove any 1 card in the infection discard pile from the game."
		ActionType.ONE_QUIET_NIGHT:
			return "Skip the next infection cities step"
		_:
			return "Unknown event."

func get_role_description(role_id: int) -> String:
	match role_id:
		RoleType.Contingency:
			return "The Contingency Planner may, as an action, take an Event card from anywhere in the Player Discard Pile and place it on his Role card. Only 1 Event card can be on his role card at a time. It does not count against his hand limit."
		RoleType.Dispatcher:
			return "The Dispatcher may, as an action, either:\n• move any pawn, if its owner agrees, to any city containing another pawn, or\n• move another player’s pawn, if its owner agrees, as if it were his own"
		RoleType.Medic:
			return "The Medic removes all cubes, not 1, of the same color when doing the Treat Disease action.\nIf a disease has been cured, he automatically removes all cubes of that color from a city, simply by entering it or being there. This does not take an action."
		RoleType.Operations:
			return "The Operations Expert may, as an action, either:\n• build a research station in his current city without discarding (or using) a City card, or\n• once per turn, move from a research station to any city by discarding any City card."
		RoleType.Quarantine:
			return "The Quarantine Specialist prevents both outbreaks and the placement of disease cubes in the city she is in and all cities connected to that city. She does not affect cubes placed during setup."
		RoleType.Researcher:
			return "When doing the Share Knowledge action, the Researcher may give any City card from her hand to another player in the same city as her, without this card having to match her city. The transfer must be from her hand to the other."
		RoleType.Scientist:
			return "The Scientist needs only 4 (not 5) City cards of the same disease color to Discover a Cure for that disease."
		_:
			return "Unknown role."

enum RoleType {
	Contingency = 0,
	Dispatcher = 1,
	Medic = 2,
	Operations = 3,
	Quarantine = 4,
	Researcher = 5,
	Scientist = 6
};

enum CardType  {
	CITY = 0,
	EVENT = 1,
	EPIDEMIC = 2
};

var RoleColors = {
	RoleType.Medic: Color.CORAL,
	RoleType.Scientist: Color.GHOST_WHITE,
	RoleType.Researcher: Color.SADDLE_BROWN,
	RoleType.Operations: Color.LIGHT_GREEN,
	RoleType.Dispatcher: Color.VIOLET,
	RoleType.Contingency: Color.AQUAMARINE,
	RoleType.Quarantine: Color.DARK_GREEN
}

func get_city_color(color_enum: int) -> Color:
	match color_enum:
		CityColor.BLUE: return Color.DODGER_BLUE
		CityColor.RED: return Color.CRIMSON
		CityColor.YELLOW: return Color.GOLD
		CityColor.BLACK: return Color.DARK_SLATE_GRAY
		_: return Color.GRAY
