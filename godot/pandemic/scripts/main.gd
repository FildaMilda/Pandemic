extends Node2D

@export var player_scene: PackedScene = preload("res://scenes/player.tscn")

@onready var players = $Players
@onready var cities = $Cities
@onready var action_hud = $HUD/BottomBar/ActionMenu
@onready var hand_hud = $HUD/BottomBar/CardHUD
@onready var role_hud = $HUD/BottomBar/RoleHUD
@onready var top_bar = $HUD/TopBar
@onready var event_popup = $HUD/EventPopup
@onready var card_picker_popup = $HUD/CardPickerPopup
@onready var player_picker_popup = $HUD/PlayerPickerPopup
@onready var forecast_popup = $HUD/ForecastPopup

var game = PandemicGame.new()
var history: Array = []
var current_index: int = 0
var ai_thread: Thread

var reachable_cities : Array[int]
var city_nodes_by_id = {}
var current_player_id: int = -1
var active_dispatcher_target: int = -1

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	var start_time = Time.get_ticks_msec()
	print("[DEBUG] main.tscn spawned at: ", start_time)

	# Interesting seeds:
	# Seed: 2 - Airlift and Resilient Pop start
	# Seed: 3 - Forecast start
	game.setup_game(Globals.game_difficulty, Globals.game_players, Globals.game_seed)
	
	print("[DEBUG] C++ game.setup_game() took: ", Time.get_ticks_msec() - start_time, "ms")
	var t1 = Time.get_ticks_msec()

	_create_players()

	# 1. Setup City Click listeners and caching
	for city in cities.get_children():
		city_nodes_by_id[city.city_id] = city
		city.clicked.connect(_on_city_clicked)

	print("[DEBUG] Node caching took: ", Time.get_ticks_msec() - t1, "ms")
	var t2 = Time.get_ticks_msec()
	# 2. Setup HUD Action listener
	action_hud.action_requested.connect(_on_hud_action_requested)
	role_hud.action_requested.connect(_on_hud_action_requested)
	role_hud.dispatcher_target_selected.connect(_on_dispatcher_target_selected)
	role_hud.expert_move_toggled.connect(_on_expert_move_toggled)
	hand_hud.event_card_clicked.connect(_on_event_card_clicked)
	event_popup.execute_event_requested.connect(_on_execute_event_requested)

	if Globals.is_observe_ai:
		print("In Observe AI")
		# Add base clone as dict
		history.append({
			"game": game.clone(),
			"action": "Initial State",
			"turn_info": {}
		})
		current_index = 0
		call_deferred("_setup_ai_ui")
		
		# Start background rendering of states
		print("Starting the thread")
		ai_thread = Thread.new()
		ai_thread.start(_run_ai_game_loop)


	event_popup.canceled.connect(_on_event_canceled)
	card_picker_popup.card_selected.connect(_on_hud_action_requested)
	card_picker_popup.event_card_selected.connect(_on_event_card_clicked_from_discard)
	card_picker_popup.hidden.connect(func(): in_discard_mode = false)
	
	player_picker_popup.player_selected.connect(_on_airlift_player_selected)
	player_picker_popup.canceled.connect(_cancel_airlift)
	
	forecast_popup.forecast_applied.connect(_on_forecast_applied)
	forecast_popup.canceled.connect(_cancel_forecast)

	var t3 = Time.get_ticks_msec()
	draw_state(game)
	print("[DEBUG] draw_state() initial render took: ", Time.get_ticks_msec() - t3, "ms")
	print("[DEBUG] Total main.tscn _ready initialization took: ", Time.get_ticks_msec() - start_time, "ms")




func _setup_ai_ui() -> void:
	# Keep simple action HUD but convert it or add buttons to it
	action_hud.hide() # Or setup observe controls
	
	# Create Observe UI Container
	var hbox = HBoxContainer.new()
	hbox.name = "HBoxContainer"
	$HUD/BottomBar.add_child(hbox)
	
	var prev_btn = Button.new()
	prev_btn.text = "Previous State"
	prev_btn.pressed.connect(_on_prev_state)
	hbox.add_child(prev_btn)
	
	var next_btn = Button.new()
	next_btn.text = "Next State"
	next_btn.pressed.connect(_on_next_state)
	hbox.add_child(next_btn)
	
	var label = Label.new()
	label.name = "StateLabel"
	label.text = "State 1 / 1"
	hbox.add_child(label)
	
	var action_label = Label.new()
	action_label.name = "ActionLabel"
	action_label.text = "Action: Initial State"
	action_label.add_theme_color_override("font_color", Color(1, 1, 0)) # highlight
	hbox.add_child(action_label)

func _update_state_label() -> void:
	if has_node("HUD/BottomBar/HBoxContainer/StateLabel"):
		$HUD/BottomBar/HBoxContainer/StateLabel.text = "State %d / %d" % [current_index + 1, history.size()]

func _on_prev_state() -> void:
	if current_index > 0:
		current_index -= 1
		_apply_history_state()

func _on_next_state() -> void:
	if current_index < history.size() - 1:
		current_index += 1
		_apply_history_state()

func _apply_history_state() -> void:
	var dict = history[current_index]
	game = dict["game"]
	_update_state_label()
	
	if has_node("HUD/BottomBar/HBoxContainer/ActionLabel"):
		$HUD/BottomBar/HBoxContainer/ActionLabel.text = "Action: " + dict.get("action", "")
		
	if dict.has("turn_info"):
		var t_info = dict["turn_info"]
		if t_info.has("drawn_player_cards"):
			top_bar._last_player_cards = t_info["drawn_player_cards"]
		if t_info.has("drawn_infection_cards"):
			top_bar._last_infection_cards = t_info["drawn_infection_cards"]
	
	draw_state(game)

func _run_ai_game_loop():
	var current_node = history.back()["game"]

	# While game is running
	print("Starting the game")
	while not current_node.is_game_over():
		var state_clone = current_node.clone()
		var actions = state_clone.get_mcts_macro_action(10000) # choose iterations
		state_clone.call_deferred("free")

		if actions.size() == 0:
			push_warning("MCTS returned no actions, stopping AI loop")
			break

		for action in actions:
			var step_clone = current_node.clone()
			
			var act_str = ""
			if step_clone.has_method("get_action_string"):
				act_str = step_clone.get_action_string(action)
			else:
				act_str = "Action " + str(action)
				
			print("[DEBUG] AI Executing Action: {} ({})", act_str, action)
			var t_info = step_clone.execute_action(action)

			current_node = step_clone

			var dict = {
				"game": step_clone,
				"action": act_str,
				"turn_info": t_info
			}

			# Lock array if updating while main thread accesses it
			call_deferred("_append_history_state", dict)
func _append_history_state(new_state):
	history.append(new_state)
	_update_state_label()

func _notification(what: int) -> void:
	if what == NOTIFICATION_PREDELETE:
		if ai_thread and ai_thread.is_started():
			ai_thread.wait_to_finish()
		for dict in history:
			if is_instance_valid(dict["game"]):
				dict["game"].free()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func draw_state(state: PandemicGame) -> void:
	top_bar.update_stats(state)
	var new_player_id = state.get_current_player()
	if new_player_id != current_player_id:
		current_player_id = new_player_id
		
		# Reset dispatcher target when player changes
		if state.get_player_role(current_player_id) == Globals.RoleType.Dispatcher:
			active_dispatcher_target = current_player_id
		else:
			active_dispatcher_target = -1
			
		_focus_camera_on_current_player(state)
		
	_update_city_actions(state)
	_update_cities(state)
	_update_players_location(state)
	_highlight_reachable_cities(state)
	hand_hud.update_hand(state)

func _focus_camera_on_current_player(state: PandemicGame):
	var city_id = state.get_player_location(current_player_id)
	var city_node = city_nodes_by_id.get(city_id)
	if city_node:
		var cam = get_node_or_null("Camera2D")
		if cam and cam.has_method("focus_on_position"):
			cam.focus_on_position(city_node.global_position, 1.8)

func _update_cities(state: PandemicGame):
	var stations = []
	if state.has_method("get_stations"):
		stations = state.get_stations()
		
	var hotspots = []
	if state.has_method("get_hotspots"):
		hotspots = state.get_hotspots()

	for city in cities.get_children():
		var id = city.city_id
		city.set_counter(Globals.CityColor.BLUE, state.get_city_infection(id, Globals.CityColor.BLUE))
		city.set_counter(Globals.CityColor.YELLOW, state.get_city_infection(id, Globals.CityColor.YELLOW))
		city.set_counter(Globals.CityColor.BLACK, state.get_city_infection(id, Globals.CityColor.BLACK))
		city.set_counter(Globals.CityColor.RED, state.get_city_infection(id, Globals.CityColor.RED))
		city.has_station = stations.has(id)
		city.is_hotspot = hotspots.has(id)

func _on_city_clicked(city_id: int, city_name: String):
	if in_government_grant_mode:
		if available_gov_grant_actions.has(city_id):
			var turn_results = game.execute_action(available_gov_grant_actions[city_id])
			top_bar.turn_finished(turn_results)
			in_government_grant_mode = false
			active_event_card_id = -1
			available_gov_grant_actions.clear()
			draw_state(game)
		else:
			# Cancel event if invalid city clicked
			in_government_grant_mode = false
			active_event_card_id = -1
			available_gov_grant_actions.clear()
			_highlight_reachable_cities(game)
		return
		
	if in_airlift_mode and active_airlift_player != -1:
		if available_airlift_actions.has(active_airlift_player) and available_airlift_actions[active_airlift_player].has(city_id):
			var turn_results = game.execute_action(available_airlift_actions[active_airlift_player][city_id])
			top_bar.turn_finished(turn_results)
			_cancel_airlift()
			draw_state(game)
		else:
			# Re-click logic or cancel? Let's just cancel
			_cancel_airlift()
		return
		
	if in_expert_move_mode:
		role_hud.show_expert_move_discard(city_id)
		return
		
	var city_node = city_nodes_by_id[city_id]
	if city_node:
		action_hud.open(city_id, city_name, city_node.color_type)
	else:
		action_hud.open(city_id, city_name)
	action_hud.show()
	
func _create_players():
	for i in range(game.get_player_count()):
		var new_player = player_scene.instantiate()
		players.add_child(new_player)
		new_player.setup(i, game.get_player_role(i))
	
func _on_hud_action_requested(action : int):
	var turn_results: Dictionary = game.execute_action(action)
	top_bar.turn_finished(turn_results)
	print(turn_results)
	draw_state(game)

func _on_dispatcher_target_selected(player_id: int):
	active_dispatcher_target = player_id
	_update_city_actions(game)
	_highlight_reachable_cities(game)

var in_expert_move_mode: bool = false
var in_government_grant_mode: bool = false
var in_airlift_mode: bool = false
var active_airlift_player: int = -1
var available_airlift_actions: Dictionary = {}

var active_event_card_id: int = -1
var available_gov_grant_actions: Dictionary = {}
var in_discard_mode: bool = false

func _on_event_card_clicked_from_discard(card_id: int):
	# Bypass the discard mode block
	if game.get_card_type(card_id) == Globals.CardType.EVENT:
		var name = game.get_card_name(card_id)
		var event_action_type = game.get_event_action_id(card_id)
		var desc = Globals.get_event_description(event_action_type)
		event_popup.open(card_id, name, desc)

func _on_event_card_clicked(card_id: int):
	if in_discard_mode:
		print("Ignored event click because discard popup is open")
		return
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
				var turn_results = game.execute_action(action)
				top_bar.turn_finished(turn_results)
				draw_state(game)
				return
	
	elif event_action_type == Globals.ActionType.GOVERNMENT_GRANT:
		in_government_grant_mode = true
		active_event_card_id = card_id
		available_gov_grant_actions.clear()
		
		for action in allowed_actions:
			if (action & 0x1F) == Globals.ActionType.GOVERNMENT_GRANT:
				var target_city = get_target_city(action)
				available_gov_grant_actions[target_city] = action
				
		_highlight_reachable_cities(game)

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

func _on_event_canceled():
	_update_city_actions(game)
	_highlight_reachable_cities(game)

func _on_airlift_player_selected(player_id: int):
	active_airlift_player = player_id
	_highlight_reachable_cities(game)

func _cancel_airlift():
	in_airlift_mode = false
	active_airlift_player = -1
	active_event_card_id = -1
	available_airlift_actions.clear()
	_update_city_actions(game)
	_highlight_reachable_cities(game)

func _on_forecast_applied(mapping: Array):
	var padded = []
	for i in range(6):
		padded.append(mapping[i] if i < mapping.size() else 0)
	
	game.do_forecast(padded[0], padded[1], padded[2], padded[3], padded[4], padded[5])
	active_event_card_id = -1
	draw_state(game)

func _cancel_forecast():
	# If we need to revert active event card
	active_event_card_id = -1
	_update_city_actions(game)
	draw_state(game)

func _on_expert_move_toggled(active: bool):
	in_expert_move_mode = active
	_highlight_reachable_cities(game)

func get_type(raw_data: int) -> int:
	return raw_data & 0x1F # 0x1F is 31 (the first 5 bits)

# Helper function to get the 'target_city' (6 bits starting at bit 5)
func get_target_city(raw_data: int) -> int:
	return (raw_data >> 5) & 0x3F # 0x3F is 63 (6 bits)

func get_target_player(raw_data: int) -> int:
	return (raw_data >> 13) & 0x3 # 0x3 is 3 (2 bits)
		
func treat_get_color_id(raw_data: int) -> int:
	return (raw_data >> 13) & 0x3
		
func _update_players_location(state: PandemicGame):
	for player_id in state.get_player_count():
		var city_id : int = state.get_player_location(player_id)
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
		
func _update_city_actions(state: PandemicGame):
	var allowed_actions: Array = state.get_possible_actions()
	
	var p = state.get_current_player()
	print("--- UPDATING ACTIONS FOR PLAYER ", p, " ---")
	print("Action count: ", allowed_actions.size())
	
	reachable_cities.clear()
	action_hud.reset_actions()
	role_hud.update_hud(state, p, allowed_actions, active_dispatcher_target)
	
	if allowed_actions.size() > 0:
		var has_discard = false
		var has_remove_station = false
		var only_remove_station = true
		
		var discard_actions = {}
		var remove_station_actions = {}
		var forced_player = -1
		
		for action in allowed_actions:
			var type: int = get_type(action)
			
			if type == Globals.ActionType.DISCARD_CARD:
				has_discard = true
				forced_player = (action >> 11) & 0x3
				var target_city = get_target_city(action)
				discard_actions[target_city] = action
			elif type == Globals.ActionType.REMOVE_STATION:
				has_remove_station = true
				forced_player = (action >> 11) & 0x3
				var target_city = get_target_city(action)
				remove_station_actions[target_city] = action
			
			if type != Globals.ActionType.REMOVE_STATION:
				only_remove_station = false
		
		if only_remove_station and has_remove_station:
			var title_text = "Player " + str(forced_player + 1) + ": Remove a Station"
			card_picker_popup.open(title_text, state, remove_station_actions)
			return
		elif has_discard:
			var title_text = "Player " + str(forced_player + 1) + ": Discard a Card"
			
			# Add playable event cards to the discard menu so they can use them directly
			var hand_ids = state.get_player_hand(forced_player)
			for c_id in hand_ids:
				if state.get_card_type(c_id) == Globals.CardType.EVENT:
					var event_key = "play_" + str(c_id)
					discard_actions[event_key] = -1
			
			in_discard_mode = true
			card_picker_popup.open(title_text, state, discard_actions)
			# Do not return here so they can still see the HUD in case they want to play events
			
	var filter_target_player = active_dispatcher_target if active_dispatcher_target != -1 else p

	for action in allowed_actions:
		var type : int = get_type(action)
		
		var target_city : int = -1
		if type != Globals.ActionType.CURE:
			target_city = get_target_city(action)
		else:
			target_city = state.get_player_location(p) # Cure doesn't encode target_city, uses current
		
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
			
func _highlight_reachable_cities(state: PandemicGame):
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
			if active_dispatcher_target != -1 and state.get_player_role(current_player_id) == Globals.RoleType.Dispatcher:
				city.highlight_color = Globals.RoleColors[Globals.RoleType.Dispatcher]
			else:
				city.highlight_color = Color(0.1, 0.8, 0.3)
			city.highlight = true
