extends PanelContainer

signal clicked(card_id: int)

@onready var card_name = $MarginContainer/HBoxContainer/VBoxContainer/CardName
@onready var color_indicator = $MarginContainer/HBoxContainer/ColorIndicator
@onready var event_badge = $MarginContainer/HBoxContainer/VBoxContainer/EventBadge

var _card_id: int = -1

func setup(card_id: int, card_name_text: String, card_type: int, color_enum: int):
	_card_id = card_id
	card_name.text = card_name_text
	
	# Show event badge if it's an event card
	if card_type == Globals.CardType.EVENT:
		event_badge.visible = true
	else:
		event_badge.visible = false
	
	# Get color from enum
	var display_color = _get_color_from_enum(card_type, color_enum)
	
	# Create a styled panel with border
	var panel_style = StyleBoxFlat.new()
	panel_style.bg_color = display_color.darkened(0.7) # Using darkened() is safer and avoids HSV errors
	panel_style.set_border_width_all(2) # Replaced the non-existent set_border_enabled
	panel_style.border_color = display_color
	panel_style.set_corner_radius_all(4)
	add_theme_stylebox_override("panel", panel_style)
	
	# Color indicator on the left
	var indicator_style = StyleBoxFlat.new()
	indicator_style.bg_color = display_color
	indicator_style.set_corner_radius_all(2)
	color_indicator.add_theme_stylebox_override("panel", indicator_style)
	
	# Style for text - make it more readable
	var font_color = Color.WHITE
	card_name.add_theme_color_override("font_color", font_color)
	if card_type == Globals.CardType.EVENT:
		event_badge.add_theme_color_override("font_color", Color.BURLYWOOD)
		self.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
		self.gui_input.connect(_on_gui_input)

func _on_gui_input(event: InputEvent) -> void:
	if event is InputEventMouseButton and event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
		clicked.emit(_card_id)

func _get_color_from_enum(card_type: int, color_enum: int) -> Color:
	if card_type == Globals.CardType.EVENT:
		return Color.BURLYWOOD
	
	match color_enum:
		Globals.CityColor.BLUE: return Color.DODGER_BLUE
		Globals.CityColor.RED: return Color.CRIMSON
		Globals.CityColor.YELLOW: return Color.GOLD
		Globals.CityColor.BLACK: return Color.DARK_SLATE_GRAY
		_: return Color.GRAY
