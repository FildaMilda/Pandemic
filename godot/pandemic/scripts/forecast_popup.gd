extends PanelContainer

signal forecast_applied(indices: Array)
signal canceled()

@onready var available_list = $VBox/HBox/AvailableSection/Scroll/List
@onready var selected_list = $VBox/HBox/SelectedSection/Scroll/List
@onready var reset_btn = $VBox/Buttons/ResetBtn
@onready var apply_btn = $VBox/Buttons/ApplyBtn
@onready var cancel_btn = $VBox/Buttons/CancelBtn

var _game: Node
var _original_cards: Array = []
var _selected_indices: Array = [] # Stores original indices of cards currently moved to new order

func _ready():
	reset_btn.pressed.connect(_on_reset)
	apply_btn.pressed.connect(_on_apply)
	cancel_btn.pressed.connect(_on_cancel)
	hide()

func open(game: Node, cards: Array):
	if cards.size() <= 1:
		push_warning("Forecast called with too few cards.")
		return
		
	_game = game
	_original_cards = cards.duplicate()
	_selected_indices.clear()
	
	_refresh_ui()
	_center_on_screen()
	show()

func _refresh_ui():
	# Clear both lists
	for c in available_list.get_children(): c.queue_free()
	for c in selected_list.get_children(): c.queue_free()
	
	# Populate available section with remaining original cards
	for i in range(_original_cards.size()):
		if not _selected_indices.has(i):
			var card_id = _original_cards[i]
			var btn = _create_card_button(card_id, "Original Top " + str(i + 1))
			btn.pressed.connect(func(): _on_select_card(i))
			available_list.add_child(btn)
			
	# Populate selected section
	for pos in range(_selected_indices.size()):
		var orig_idx = _selected_indices[pos]
		var card_id = _original_cards[orig_idx]
		var title_prefix = "Top" if pos == 0 else ("#" + str(pos + 1))
		var btn = _create_card_button(card_id, title_prefix)
		btn.pressed.connect(func(): _on_deselect_card(pos))
		selected_list.add_child(btn)
		
	apply_btn.disabled = _selected_indices.size() != _original_cards.size()

func _create_card_button(card_id: int, prefix: String) -> Button:
	var btn = Button.new()
	var card_name = "Card " + str(card_id)
	var color = Color.WHITE
	
	if _game.has_method("get_card_name"):
		card_name = _game.get_card_name(card_id)
		
	if _game.has_method("get_card_color"):
		var color_enum = _game.get_card_color(card_id)
		color = _get_border_color(color_enum)
		
	btn.text = prefix + ": " + card_name
	btn.custom_minimum_size = Vector2(0, 34)
	
	var style = StyleBoxFlat.new()
	style.bg_color = Color(0.12, 0.12, 0.12, 1.0)
	style.border_color = color
	style.set_border_width_all(2)
	style.set_corner_radius_all(4)
	btn.add_theme_stylebox_override("normal", style)
	
	var hover = style.duplicate()
	hover.bg_color = Color(0.25, 0.25, 0.25, 1.0)
	btn.add_theme_stylebox_override("hover", hover)
	
	btn.mouse_default_cursor_shape = Control.CURSOR_POINTING_HAND
	return btn

func _get_border_color(color_enum: int) -> Color:
	match color_enum:
		Globals.CityColor.BLUE: return Color.DODGER_BLUE
		Globals.CityColor.RED: return Color.CRIMSON
		Globals.CityColor.YELLOW: return Color.GOLD
		Globals.CityColor.BLACK: return Color.DARK_SLATE_GRAY
		_: return Color.GRAY

func _on_select_card(orig_idx: int):
	_selected_indices.append(orig_idx)
	_refresh_ui()

func _on_deselect_card(selected_pos: int):
	_selected_indices.remove_at(selected_pos)
	_refresh_ui()

func _on_reset():
	_selected_indices.clear()
	_refresh_ui()

func _on_apply():
	hide()
	forecast_applied.emit(_selected_indices.duplicate())

func _on_cancel():
	hide()
	canceled.emit()

func _center_on_screen() -> void:
	var screen_size = get_viewport_rect().size
	global_position = (screen_size - size) / 2.0