extends Control

@onready var difficulty_option = $VBoxContainer/DifficultyOption
@onready var players_option = $VBoxContainer/PlayersOption
@onready var seed_input = $VBoxContainer/SeedInput
@onready var play_button = $VBoxContainer/PlayButton
@onready var observe_button = $VBoxContainer/ObserveButton

var ready_scene_instance: Node = null

func _ready() -> void:
	call_deferred("_pre_build_scene")
	
	# Setup Difficulty Options
	difficulty_option.add_item("Introductory", 0)
	difficulty_option.add_item("Standard", 1)
	difficulty_option.add_item("Heroic", 2)
	difficulty_option.selected = 1 # Default to Standard
	
	# Setup Players Options
	players_option.add_item("2 Players", 2)
	players_option.add_item("3 Players", 3)
	players_option.add_item("4 Players", 4)
	players_option.selected = 2 # Default to 4 Players
	
	# Setup Seed Input
	randomize()
	var default_seed = randi() % 1000000
	seed_input.text = str(default_seed)
	
	# Connect signals
	play_button.pressed.connect(_on_play_pressed)
	observe_button.pressed.connect(_on_observe_pressed)

func _pre_build_scene() -> void:
	# Build the nodes in memory while the user is looking at the menu
	ready_scene_instance = Globals.preloaded_main_scene.instantiate()

func _on_play_pressed() -> void:
	print("[DEBUG] Play Button click registered at: ", Time.get_ticks_msec())
	Globals.is_observe_ai = false
	_save_settings_to_globals()
	
	if ready_scene_instance != null:
		var root = get_tree().root
		get_tree().current_scene.queue_free()
		root.add_child(ready_scene_instance)
		get_tree().current_scene = ready_scene_instance
	else:
		# Switch instantly using the preloaded scene stored in Globals
		get_tree().change_scene_to_packed(Globals.preloaded_main_scene)

func _on_observe_pressed() -> void:
	print("[DEBUG] Observe Button click registered at: ", Time.get_ticks_msec())
	Globals.is_observe_ai = true
	_save_settings_to_globals()
	
	if ready_scene_instance != null:
		var root = get_tree().root
		get_tree().current_scene.queue_free()
		root.add_child(ready_scene_instance)
		get_tree().current_scene = ready_scene_instance
	else:
		# Switch instantly using the preloaded scene stored in Globals
		get_tree().change_scene_to_packed(Globals.preloaded_main_scene)

func _save_settings_to_globals() -> void:
	Globals.game_difficulty = difficulty_option.get_item_id(difficulty_option.selected)
	Globals.game_players = players_option.get_item_id(players_option.selected)
	
	var seed_text = seed_input.text.strip_edges()
	if seed_text.is_valid_int():
		Globals.game_seed = seed_text.to_int()
	else:
		Globals.game_seed = seed_text.hash() # fallback to a hash if it's a string
