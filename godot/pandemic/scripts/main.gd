extends Node2D

@export var player_scene: PackedScene = preload("res://scenes/player.tscn")

@onready var players = $Players
@onready var cities = $Cities
@onready var action_hud = $HUD/ActionMenu
@onready var hand_hud = $HUD/CardHUD
@onready var role_hud = $HUD/RoleHUD
@onready var event_popup = $HUD/EventPopup
@onready var card_picker_popup = $HUD/CardPickerPopup
@onready var player_picker_popup = $HUD/PlayerPickerPopup
@onready var forecast_popup = $HUD/ForecastPopup

var game = PandemicGame.new()
var reachable_cities : Array[int]
var city_nodes_by_id = {}
var current_player_id: int = -1
var active_dispatcher_target: int = -1

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	# Interesting seeds:
	# Seed: 2 - Airlift and Resilient Pop start
	# Seed: 3 - Forecast start
	game.setup_game(42)
	
	_create_players()

	# 1. Setup City Click listeners and caching
	for city in cities.get_children():
		city_nodes_by_id[city.city_id] = city
		city.clicked.connect(_on_city_clicked)

	# 2. Setup HUD Action listener
	action_hud.action_requested.connect(_on_hud_action_requested)
	role_hud.action_requested.connect(_on_hud_action_requested)
	role_hud.dispatcher_target_selected.connect(_on_dispatcher_target_selected)
	role_hud.expert_move_toggled.connect(_on_expert_move_toggled)
	hand_hud.event_card_clicked.connect(_on_event_card_clicked)
	event_popup.execute_event_requested.connect(_on_execute_event_requested)
	card_picker_popup.card_selected.connect(_on_hud_action_requested)
	
	player_picker_popup.player_selected.connect(_on_airlift_player_selected)
	player_picker_popup.canceled.connect(_cancel_airlift)
	
	forecast_popup.forecast_applied.connect(_on_forecast_applied)
	forecast_popup.canceled.connect(_cancel_forecast)

	_update()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _update():
	var new_player_id = game.get_current_player()
	if new_player_id != current_player_id:
		current_player_id = new_player_id
		
		# Reset dispatcher target when player changes
		if game.get_player_role(current_player_id) == Globals.RoleType.Dispatcher:
			active_dispatcher_target = current_player_id
		else:
			active_dispatcher_target = -1
			
		_focus_camera_on_current_player()
		
	_update_city_actions()
	_update_cities()
	_update_players_location()
	_highlight_reachable_cities()
	hand_hud.update_hand(game)

func _focus_camera_on_current_player():
	var city_id = game.get_player_location(current_player_id)
	var city_node = city_nodes_by_id.get(city_id)
	if city_node:
		var cam = get_node_or_null("Camera2D")
		if cam and cam.has_method("focus_on_position"):
			cam.focus_on_position(city_node.global_position, 1.8)

func _update_cities():
	var stations = []
	if game.has_method("get_stations"):
		stations = game.get_stations()
		
	for city in cities.get_children():
		var id = city.city_id
		city.set_counter(Globals.CityColor.BLUE, game.get_city_infection(id, Globals.CityColor.BLUE))
		city.set_counter(Globals.CityColor.YELLOW, game.get_city_infection(id, Globals.CityColor.YELLOW))
		city.set_counter(Globals.CityColor.BLACK, game.get_city_infection(id, Globals.CityColor.BLACK))
		city.set_counter(Globals.CityColor.RED, game.get_city_infection(id, Globals.CityColor.RED))
		city.has_station = stations.has(id)

func _on_city_clicked(city_id: int, city_name: String):
	if in_government_grant_mode:
		if available_gov_grant_actions.has(city_id):
			game.execute_action(available_gov_grant_actions[city_id])
			in_government_grant_mode = false
			active_event_card_id = -1
			available_gov_grant_actions.clear()
			_update()
		else:
			# Cancel event if invalid city clicked
			in_government_grant_mode = false
			active_event_card_id = -1
			available_gov_grant_actions.clear()
			_highlight_reachable_cities()
		return
		
	if in_airlift_mode and active_airlift_player != -1:
		if available_airlift_actions.has(active_airlift_player) and available_airlift_actions[active_airlift_player].has(city_id):
			game.execute_action(available_airlift_actions[active_airlift_player][city_id])
			_cancel_airlift()
			_update()
		else:
			# Re-click logic or cancel? Let's just cancel
			_cancel_airlift()
		return
		
	if in_expert_move_mode:
		role_hud.show_expert_move_discard(city_id)
		return
		
	action_hud.open(city_id, city_name)
	action_hud.show()
	
func _create_players():
	for i in range(game.get_player_count()):
		var new_player = player_scene.instantiate()
		players.add_child(new_player)
		new_player.setup(i, game.get_player_role(i))
	
func _on_hud_action_requested(action : int):
	game.execute_action(action)
	_update()

func _on_dispatcher_target_selected(player_id: int):
	active_dispatcher_target = player_id
	_update_city_actions()
	_highlight_reachable_cities()

var in_expert_move_mode: bool = false
var in_government_grant_mode: bool = false
var in_airlift_mode: bool = false
var active_airlift_player: int = -1
var available_airlift_actions: Dictionary = {}

var active_event_card_id: int = -1
var available_gov_grant_actions: Dictionary = {}

func _on_event_card_clicked(card_id: int):
	if game.get_card_type(card_id) == Globals.CardType.EVENT:
		var name = game.get_card_name(card_id)
		var event_action_type = game.get_event_action_id(card_id)
		var desc = Globals.get_event_description(event_action_type)
		event_popup.open(card_id, name, desc)

func _on_execute_event_requested(card_id: int):		
	var event_action_type = game.get_event_action_id(card_id)
	var allowed_actions = game.get_possible_actions()
	
	if event_action_type == Globals.ActionType.ONE_QUIET_NIGHT:
		for action in allowed_actions:
			if (action & 0x1F) == Globals.ActionType.ONE_QUIET_NIGHT:
				game.execute_action(action)
				_update()
				return
	
	elif event_action_type == Globals.ActionType.GOVERNMENT_GRANT:
		in_government_grant_mode = true
		active_event_card_id = card_id
		available_gov_grant_actions.clear()
		
		for action in allowed_actions:
			if (action & 0x1F) == Globals.ActionType.GOVERNMENT_GRANT:
				var target_city = get_target_city(action)
				available_gov_grant_actions[target_city] = action
				
		_highlight_reachable_cities()

	elif event_action_type == Globals.ActionType.FORECAST:
		# Check if action is allowed
		var valid_action = -1
		for action in allowed_actions:
			print("I found Forecast action")
			if (action & 0x1F) == Globals.ActionType.FORECAST:
				valid_action = action
				break
		if valid_action != -1:
			var cards = game.get_forecast_cards()
			print(cards.size())
			if cards.size() > 1:
				forecast_popup.open(game, cards)
			else:
				# Auto discard if 1 or 0 card and we just execute action?
				# Actually let's assume get_forecast_cards handles it
				pass
				
	elif event_action_type == Globals.ActionType.RESILIENT_POPULATION:
		var available_resilient_actions = {}
		
		for action in allowed_actions:
			if (action & 0x1F) == Globals.ActionType.RESILIENT_POPULATION:
				var discard_card = get_target_city(action)
				available_resilient_actions[discard_card] = action
				
		if not available_resilient_actions.is_empty():
			card_picker_popup.open("Select Card to Remove", game, available_resilient_actions)
			
	elif event_action_type == Globals.ActionType.AIRLIFT:
		# Start airlift pipeline by choosing player
		in_airlift_mode = true
		active_event_card_id = card_id
		
		# Gather all valid airlift player targets before opening popup
		available_airlift_actions.clear()
		for action in allowed_actions:
			if (action & 0x1F) == Globals.ActionType.AIRLIFT:
				var t_player = get_target_player(action)
				var t_city = get_target_city(action)
				
				if not available_airlift_actions.has(t_player):
					available_airlift_actions[t_player] = {}
				available_airlift_actions[t_player][t_city] = action
				
		if not available_airlift_actions.is_empty():
			player_picker_popup.open("Select Player to Airlift", game)
		else:
			_cancel_airlift()

func _on_airlift_player_selected(player_id: int):
	active_airlift_player = player_id
	_highlight_reachable_cities()

func _cancel_airlift():
	in_airlift_mode = false
	active_airlift_player = -1
	active_event_card_id = -1
	available_airlift_actions.clear()
	_highlight_reachable_cities()

func _on_forecast_applied(mapping: Array):
	var padded = []
	for i in range(6):
		padded.append(mapping[i] if i < mapping.size() else 0)
	
	game.do_forecast(padded[0], padded[1], padded[2], padded[3], padded[4], padded[5])
	active_event_card_id = -1
	_update()

func _cancel_forecast():
	# If we need to revert active event card
	active_event_card_id = -1
	_update()

func _on_expert_move_toggled(active: bool):
	in_expert_move_mode = active
	_highlight_reachable_cities()

func get_type(raw_data: int) -> int:
	return raw_data & 0x1F # 0x1F is 31 (the first 5 bits)

# Helper function to get the 'target_city' (6 bits starting at bit 5)
func get_target_city(raw_data: int) -> int:
	return (raw_data >> 5) & 0x3F # 0x3F is 63 (6 bits)

func get_target_player(raw_data: int) -> int:
	return (raw_data >> 13) & 0x3 # 0x3 is 3 (2 bits)
		
func treat_get_color_id(raw_data: int) -> int:
	return (raw_data >> 13) & 0x3
		
func _update_players_location():
	for player_id in game.get_player_count():
		var city_id : int = game.get_player_location(player_id)
		var city_node = city_nodes_by_id.get(city_id)
		
		# Important fix: The Players container doesn't guarantee the nodes are ordered 0, 1, 2, 3
		# if they were added dynamically, but our ID logic expects the child at index `player_id` to BE `player_id`.
		# Let's find the correct player node safely:
		var player = null
		for child in players.get_children():
			if child.id == player_id: # Assuming player.gd has var id or similar
				player = child
				break
				
		if not player:
			push_error("Player graphics node not found in scene tree for ID " + str(player_id))
			continue
		
		if not city_node:
			push_error("City node not found for ID " + str(city_id))
			continue
			
		var marker_path = "PlayerPositions/P" + str(player_id + 1)
		var slot = city_node.get_node_or_null(marker_path)
		
		if slot:
			player.global_position = slot.global_position
			player.z_index = 10 + player_id # Make sure active players draw on top uniquely
		else:
			push_error("Start marker not found for Player " + str(player_id))
		
func _update_city_actions():
	var allowed_actions: Array = game.get_possible_actions()
	
	var p = game.get_current_player()
	print("--- UPDATING ACTIONS FOR PLAYER ", p, " ---")
	print("Action count: ", allowed_actions.size())
	
	reachable_cities.clear()
	action_hud.reset_actions()
	role_hud.update_hud(game, p, allowed_actions, active_dispatcher_target)
	
	var filter_target_player = active_dispatcher_target if active_dispatcher_target != -1 else p

	for action in allowed_actions:
		var type : int = get_type(action)
		
		var target_city : int = -1
		if type != Globals.ActionType.CURE:
			target_city = get_target_city(action)
		else:
			target_city = game.get_player_location(p) # Cure doesn't encode target_city, uses current
		
		# Temporary Debug Code to see exactly what actions C++ is giving us:
		print("DEBUG ACTION: Type=", type, " TargetCity=", target_city, " Raw=", action)
		
		if Globals.is_movement_action(type) or type == Globals.ActionType.BUILD or type == Globals.ActionType.DISPATCHER_MOVE or type == Globals.ActionType.DISPATCHER_MOVE_AS:
			var target_player = get_target_player(action)
			
			# Check moving/building actions for the selected UI target player
			if target_player == filter_target_player:
				action_hud.available_actions[type][target_city] = action
				if not reachable_cities.has(target_city):
					reachable_cities.append(target_city)
			
		elif type == Globals.ActionType.TREAT:
			var target_color : int = treat_get_color_id(action)
			
			if not action_hud.available_actions[type].has(target_city):
				action_hud.available_actions[type][target_city] = {}
			
			action_hud.available_actions[type][target_city][target_color] = action
			if not reachable_cities.has(target_city):
				reachable_cities.append(target_city)
				
		elif type == Globals.ActionType.SHARE:
			# target_city is properly extracted for share.
			# Bits 11-12 are executing_player, 14-15 are receiving_player
			var executing_player = (action >> 11) & 0x3
			if executing_player == p:
				action_hud.available_actions[type][target_city] = action
				if not reachable_cities.has(target_city):
					reachable_cities.append(target_city)
					
		elif type == Globals.ActionType.CURE:
			action_hud.available_actions[type][target_city] = action
			if not reachable_cities.has(target_city):
				reachable_cities.append(target_city)

func _refresh_city_highlights():
	for city in cities.get_children():
		city.highlight = false
			
func _highlight_reachable_cities():
	_refresh_city_highlights()
	
	if in_government_grant_mode:
		for city_id in available_gov_grant_actions.keys():
			var city = city_nodes_by_id.get(city_id)
			if city:
				city.highlight_color = Color.BURLYWOOD
				city.highlight = true
		return
		
	if in_airlift_mode and active_airlift_player != -1:
		if available_airlift_actions.has(active_airlift_player):
			for city_id in available_airlift_actions[active_airlift_player].keys():
				var city = city_nodes_by_id.get(city_id)
				if city:
					city.highlight_color = Color.BURLYWOOD
					city.highlight = true
		return
		
	if in_expert_move_mode:
		for city_id in role_hud.available_expert_moves.keys():
			var city = city_nodes_by_id.get(city_id)
			if city:
				city.highlight_color = Globals.RoleColors[Globals.RoleType.Operations]
				city.highlight = true
		return
		
	for city_id in reachable_cities:
		var city = city_nodes_by_id.get(city_id)
		if city:
			if active_dispatcher_target != -1 and game.get_player_role(current_player_id) == Globals.RoleType.Dispatcher:
				city.highlight_color = Globals.RoleColors[Globals.RoleType.Dispatcher]
			else:
				city.highlight_color = Color(0.1, 0.8, 0.3)
			city.highlight = true
