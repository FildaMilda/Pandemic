extends Node2D

@export var player_scene: PackedScene = preload("res://scenes/player.tscn")

@onready var players = $Players
@onready var cities = $Cities
@onready var action_hud = $HUD/ActionMenu
@onready var hand_hud = $HUD/CardHUD

var game = PandemicGame.new()
var reachable_cities : Array[int]

# Called when the node enters the scene tree for the first time.
func _ready() -> void:
	game.setup_game(42)
	
	# 1. Setup City Click listeners
	for city in cities.get_children():
		city.clicked.connect(_on_city_clicked)

	# 2. Setup HUD Action listener
	action_hud.action_requested.connect(_on_hud_action_requested)
	action_hud.hide()
	
	_create_players()
	_update()
	draw_map_connections()

# Called every frame. 'delta' is the elapsed time since the previous frame.
func _process(delta: float) -> void:
	pass

func _update():
	_update_city_actions()
	_update_cities()
	_update_players_location()
	_highlight_reachable_cities()
	hand_hud.update_hand(game)

func _update_cities():
	for city in cities.get_children():
		city.set_counter(Globals.CityColor.BLUE, game.get_city_infection(city.city_id, Globals.CityColor.BLUE))
		city.set_counter(Globals.CityColor.YELLOW, game.get_city_infection(city.city_id, Globals.CityColor.YELLOW))
		city.set_counter(Globals.CityColor.BLACK, game.get_city_infection(city.city_id, Globals.CityColor.BLACK))
		city.set_counter(Globals.CityColor.RED, game.get_city_infection(city.city_id, Globals.CityColor.RED))

func _on_city_clicked(city_id: int, city_name: String):
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

func get_type(raw_data: int) -> int:
	return raw_data & 0x1F # 0x1F is 31 (the first 5 bits)

# Helper function to get the 'target_city' (6 bits starting at bit 5)
func get_target_city(raw_data: int) -> int:
	return (raw_data >> 5) & 0x3F # 0x3F is 63 (6 bits)
		
func treat_get_color_id(raw_data: int) -> int:
	return (raw_data >> 13) & 0x3
		
func _update_players_location():
	for player_id in game.get_player_count():
		var city_id : int = game.get_player_location(player_id)
		var city_node = cities.get_child(city_id)
		var player = players.get_child(player_id)
		
		var marker_path = "PlayerPositions/P" + str(player_id + 1)
		var slot = city_node.get_node(marker_path)
		
		if slot:
			player.global_position = slot.global_position
		else:
			push_error("Start marker not found for Player " + str(player.id))
		
func _update_city_actions():
	var allowed_actions: Array = game.get_possible_actions()
	reachable_cities.clear()
	action_hud.reset_actions()
	
	for action in allowed_actions:
		var type : int = get_type(action)
		
		if Globals.is_movement_action(type):
			var target_city : int = get_target_city(action)
			action_hud.available_actions[type][target_city] = action
			reachable_cities.append(target_city)
			
		elif type == Globals.ActionType.TREAT:
			var target_city : int = get_target_city(action)
			var target_color : int = treat_get_color_id(action)
			
			if not action_hud.available_actions[type].has(target_city):
				action_hud.available_actions[type][target_city] = {}
			
			action_hud.available_actions[type][target_city][target_color] = action
			
func _refresh_city_highlights():
	for city in cities.get_children():
		city.highlight = false
			
func _highlight_reachable_cities():
	_refresh_city_highlights()
	for city_id in reachable_cities:
		var city = cities.get_child(city_id)
		city.highlight = true
		
func draw_map_connections():
	print("Drawing connections")
	# Keep track of connections we've already drawn to avoid double-drawing 
	# (e.g. Atlanta -> Chicago and Chicago -> Atlanta)
	var drawn_connections = {}

	for city_id in range(48): # Total number of cities
		var neighbors = game.get_city_neighbors(city_id)
		var pos_a = cities.get_child(city_id).global_position
		
		for neighbor_id in neighbors:
			# Create a unique key for this pair regardless of order
			var pair = [city_id, neighbor_id]
			pair.sort()
			var key = str(pair[0]) + "_" + str(pair[1])
			
			if not drawn_connections.has(key):
				_create_line(pos_a, cities.get_child(city_id).global_position)
				drawn_connections[key] = true

func _create_line(from: Vector2, to: Vector2):
	var line = Line2D.new()
	add_child(line)
	line.add_point(from)
	line.add_point(to)
	line.width = 2.0
	line.default_color = Color(1, 1, 1, 0.2) # Subtle white
	line.z_index = 10 # Draw behind cities and players
	line.z_as_relative = false
