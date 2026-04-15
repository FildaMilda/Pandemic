extends PanelContainer

signal card_selected(raw_action: int)
signal event_card_selected(card_id: int)

@onready var header = $VBox/Header
@onready var grid = $VBox/ScrollContainer/GridContainer
@onready var cancel_btn = $VBox/CancelButton

var available_actions = {}
var game: Node

func _ready():
	cancel_btn.pressed.connect(hide)
	hide()

func open(title: String, _game: Node, actions_dict: Dictionary):
	header.text = title
	game = _game
	available_actions = actions_dict
	
	_populate_grid()
	_center_on_screen()
	show()

func _populate_grid():
	for child in grid.get_children():
		child.queue_free()
		
	# actions_dict is mapped to raw_action. The key can be int (card_id) or String (like "play_48").
	for key in available_actions.keys():
		var btn = Button.new()
		
		# Allow the caller to encode custom logic or text by using a Dictionary as the value,
		# or keep the simple card_id -> raw_action mapping for backward compatibility.
		var raw_action = available_actions[key]
		var card_id = -1
		var is_play_event = false
		
		if typeof(key) == TYPE_STRING and key.begins_with("play_"):
			card_id = key.trim_prefix("play_").to_int()
			is_play_event = true
		elif typeof(key) == TYPE_INT:
			card_id = key
			
		var card_name = "Card " + str(card_id)
		var color = Color.WHITE
		
		if card_id != -1:
			if game.has_method("get_card_name"):
				card_name = game.get_card_name(card_id)
				
			if game.has_method("get_card_color"):
				var color_enum = game.get_card_color(card_id)
				# Fallback, Event cards might be color enum -1, so check
				# Assumes disease color mapping exists in Globals
				color = _get_border_color(color_enum)
		
		if is_play_event:
			card_name = "PLAY Event: " + card_name
		elif typeof(key) == TYPE_INT:
			# If we are in discard mode and it's a regular card, maybe specify Discard?
			# But we want to preserve other usages that might use this popup.
			pass

		btn.text = card_name
		btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		btn.custom_minimum_size = Vector2(160, 30)
		
		# Set a border of the appropriate color
		var style = StyleBoxFlat.new()
		# Tint the background with the card color so it's very clear
		style.bg_color = color.lerp(Color(0.1, 0.1, 0.1, 1.0), 0.7)
		style.border_color = color
		style.set_border_width_all(2)
		style.set_corner_radius_all(4)
		btn.add_theme_stylebox_override("normal", style)
		
		var hover_style = style.duplicate()
		hover_style.bg_color = color.lerp(Color(0.2, 0.2, 0.2, 1.0), 0.5)
		btn.add_theme_stylebox_override("hover", hover_style)
		
		var pressed_style = style.duplicate()
		pressed_style.bg_color = color.lerp(Color(0.05, 0.05, 0.05, 1.0), 0.8)
		btn.add_theme_stylebox_override("pressed", pressed_style)
		
		btn.pressed.connect(func(): _on_card_selected(key, card_id, is_play_event))
		grid.add_child(btn)

func _get_border_color(color_enum: int) -> Color:
	match color_enum:
		Globals.CityColor.BLUE: return Color.DODGER_BLUE
		Globals.CityColor.RED: return Color.CRIMSON
		Globals.CityColor.YELLOW: return Color.GOLD
		Globals.CityColor.BLACK: return Color.DARK_SLATE_GRAY
		_: return Color.GRAY

func _on_card_selected(key, card_id: int, is_play_event: bool):
	hide()
	var raw_action = available_actions[key]
	if raw_action == -1 or is_play_event:
		event_card_selected.emit(card_id)
	else:
		card_selected.emit(raw_action)

func _center_on_screen() -> void:
	var screen_size = get_viewport_rect().size
	global_position = (screen_size - size) / 2.0