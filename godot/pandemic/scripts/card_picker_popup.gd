extends PanelContainer

signal card_selected(raw_action: int)

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
		
	# actions_dict is card_id -> raw_action
	for card_id in available_actions.keys():
		var btn = Button.new()
		var card_name = "Card " + str(card_id)
		var color = Color.WHITE
		
		if game.has_method("get_card_name"):
			card_name = game.get_card_name(card_id)
			
		if game.has_method("get_card_color"):
			var color_enum = game.get_card_color(card_id)
			# Fallback, Event cards might be color enum -1, so check
			# Assumes disease color mapping exists in Globals
			color = _get_border_color(color_enum)
			
		btn.text = card_name
		btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
		btn.custom_minimum_size = Vector2(160, 30)
		
		# Set a border of the appropriate color
		var style = StyleBoxFlat.new()
		style.bg_color = Color(0.15, 0.15, 0.15, 1.0)
		style.border_color = color
		style.set_border_width_all(2)
		style.set_corner_radius_all(4)
		btn.add_theme_stylebox_override("normal", style)
		
		var hover_style = style.duplicate()
		hover_style.bg_color = Color(0.25, 0.25, 0.25, 1.0)
		btn.add_theme_stylebox_override("hover", hover_style)
		
		var pressed_style = style.duplicate()
		pressed_style.bg_color = Color(0.1, 0.1, 0.1, 1.0)
		btn.add_theme_stylebox_override("pressed", pressed_style)
		
		btn.pressed.connect(func(): _on_card_selected(card_id))
		grid.add_child(btn)

func _get_border_color(color_enum: int) -> Color:
	match color_enum:
		Globals.CityColor.BLUE: return Color.DODGER_BLUE
		Globals.CityColor.RED: return Color.CRIMSON
		Globals.CityColor.YELLOW: return Color.GOLD
		Globals.CityColor.BLACK: return Color.DARK_SLATE_GRAY
		_: return Color.GRAY

func _on_card_selected(card_id: int):
	hide()
	card_selected.emit(available_actions[card_id])

func _center_on_screen() -> void:
	var screen_size = get_viewport_rect().size
	global_position = (screen_size - size) / 2.0