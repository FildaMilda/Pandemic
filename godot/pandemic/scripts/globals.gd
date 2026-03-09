extends Node

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
