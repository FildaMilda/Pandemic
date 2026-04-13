extends CanvasLayer

signal event_card_clicked(card_id: int)

@export var card_scene: PackedScene = preload("res://scenes/Card.tscn")
@onready var container = $HandUI/PanelContainer/MarginContainer/VBoxContainer/CardContainer
@onready var title_label = $HandUI/PanelContainer/MarginContainer/VBoxContainer/Header/Title
@onready var player_buttons = $HandUI/PanelContainer/MarginContainer/VBoxContainer/Header/PlayerButtons

var _game: Node
var _viewed_player: int = -1

func update_hand(game: Node):
	_game = game
	var current_p = game.get_current_player()
	
	# If this is the first update or it's a new turn, reset the view to current player
	if _viewed_player == -1 or current_p != _get_last_current_player():
		_viewed_player = current_p
		_set_last_current_player(current_p)
		
	_refresh_ui()

var _last_current_p: int = -1
func _get_last_current_player() -> int:
	return _last_current_p
func _set_last_current_player(p: int):
	_last_current_p = p

func _refresh_ui():
	if not _game:
		return
		
	# 1. Update Buttons
	var num_players = _game.get_player_count()
	var current_p = _game.get_current_player()
	
	# Create buttons if they don't exist yet
	if player_buttons.get_child_count() == 0:
		for i in range(num_players):
			var btn = Button.new()
			btn.text = "P" + str(i + 1)
			btn.custom_minimum_size = Vector2(45, 24) # Make the buttons wider
			btn.focus_mode = Control.FOCUS_NONE
			btn.pressed.connect(_on_player_button_pressed.bind(i))
			btn.add_theme_font_size_override("font_size", 12)
			player_buttons.add_child(btn)
			
	# Update button styles
	for i in range(player_buttons.get_child_count()):
		var btn = player_buttons.get_child(i)
		var role = _game.get_player_role(i)
		
		var style = StyleBoxFlat.new()
		if role in Globals.RoleColors:
			var color = Globals.RoleColors[role]
			# Change slightly if viewing or active
			if i == _viewed_player:
				style.bg_color = color.lightened(0.2)
				style.set_border_width_all(2)
				style.border_color = Color.WHITE
			else:
				style.bg_color = color.darkened(0.4)
			
			if i == current_p:
				btn.text = "P" + str(i + 1) + " *" # Indicator for active turn
			else:
				btn.text = "P" + str(i + 1)
				
		style.set_corner_radius_all(4)
		btn.add_theme_stylebox_override("normal", style)
		
		# Create hover and pressed styles explicitly based on bg_color
		var hover_style = style.duplicate()
		hover_style.bg_color = style.bg_color.lightened(0.1)
		btn.add_theme_stylebox_override("hover", hover_style)
		
		var pressed_style = style.duplicate()
		pressed_style.bg_color = style.bg_color.darkened(0.1)
		btn.add_theme_stylebox_override("pressed", pressed_style)
		
		# Ensure text readability based on background
		var luma = style.bg_color.get_luminance()
		btn.add_theme_color_override("font_color", Color.BLACK if luma > 0.5 else Color.WHITE)

	# 2. Clear current cards
	for child in container.get_children():
		container.remove_child(child)
		child.queue_free()
	
	# 3. Get data for VIEWED player
	var hand_ids = _game.get_player_hand(_viewed_player).duplicate()
	
	if _game.get_player_role(_viewed_player) == Globals.RoleType.Contingency:
		if _game.has_method("is_planner_empty") and not _game.is_planner_empty():
			var planner_card_id = _game.get_planner_slot()
			if planner_card_id != 255 and planner_card_id != -1:
				if not hand_ids.has(planner_card_id):
					hand_ids.append(planner_card_id)
	
	var hand_count = hand_ids.size()
	
	# 4. Update title with card count and player id
	var active_tag = " (Active)" if _viewed_player == current_p else ""
	title_label.text = "P%d Hand%s (%d/7)" % [(_viewed_player + 1), active_tag, hand_count]
	
	# 5. Create new cards
	for card_id in hand_ids:
		var type = _game.get_card_type(card_id)
		var name = _game.get_card_name(card_id)
		var color = _game.get_card_color(card_id)
		
		var new_card = card_scene.instantiate()
		container.add_child(new_card)
		
		new_card.setup(card_id, name, type, color)
		if type == Globals.CardType.EVENT:
			new_card.clicked.connect(_on_card_clicked)

func _on_card_clicked(card_id: int):
	event_card_clicked.emit(card_id)

func _on_player_button_pressed(player_index: int):
	if _viewed_player != player_index:
		_viewed_player = player_index
		_refresh_ui()

func _get_color_from_enum(type, color_enum) -> Color:
	if type == Globals.CardType.EVENT:
		return Color.BURLYWOOD
	
	match color_enum:
		Globals.CityColor.BLUE: return Color.DODGER_BLUE
		Globals.CityColor.RED: return Color.CRIMSON
		Globals.CityColor.YELLOW: return Color.GOLD
		Globals.CityColor.BLACK: return Color.DARK_SLATE_GRAY
		_: return Color.GRAY
