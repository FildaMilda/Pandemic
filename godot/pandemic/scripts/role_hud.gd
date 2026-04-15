extends PanelContainer

signal action_requested(action_id: int)
signal dispatcher_target_selected(player_id: int)
signal expert_move_toggled(active: bool)

@onready var role_title: Label = $MarginContainer/VBox/TopHBox/RoleTitle
@onready var info_button: Button = $MarginContainer/VBox/TopHBox/InfoButton
@onready var role_info_popup: PanelContainer = $RoleInfoPopup
@onready var role_info_desc: RichTextLabel = $RoleInfoPopup/MarginContainer/VBox/DescriptionLabel
@onready var role_info_close: Button = $RoleInfoPopup/MarginContainer/VBox/CloseButton
@onready var take_event_button: Button = $MarginContainer/VBox/TakeEventButton
@onready var expert_move_button: Button = $MarginContainer/VBox/ExpertMoveButton
@onready var event_selection: PanelContainer = $EventSelection
@onready var events_container: VBoxContainer = $EventSelection/VBox/EventsContainer
@onready var cancel_button: Button = $EventSelection/VBox/CancelButton
@onready var selection_label: Label = $EventSelection/VBox/Label

var dispatcher_container: HBoxContainer

var current_player: int = -1
var game: Node = null
var available_take_actions: Dictionary = {} # card_id -> raw_action
var available_expert_moves: Dictionary = {} # target_city -> Dictionary of discard_card_id -> raw_action

var is_expert_move_active: bool = false

func _ready() -> void:
	take_event_button.pressed.connect(_on_take_event_pressed)
	expert_move_button.pressed.connect(_on_expert_move_pressed)
	info_button.pressed.connect(_on_info_pressed)
	role_info_close.pressed.connect(func(): role_info_popup.hide())
	expert_move_button.toggle_mode = true
	cancel_button.pressed.connect(_on_cancel_pressed)
	event_selection.hide()
	role_info_popup.hide()

	dispatcher_container = HBoxContainer.new()
	dispatcher_container.alignment = BoxContainer.ALIGNMENT_CENTER
	dispatcher_container.hide()
	$MarginContainer/VBox.add_child(dispatcher_container)

func _on_info_pressed() -> void:
	if current_player != -1 and game != null:
		var role = game.get_player_role(current_player)
		role_info_desc.text = Globals.get_role_description(role)
		
		# Center popup nicely
		var screen_size = get_viewport_rect().size
		role_info_popup.global_position = (screen_size - role_info_popup.size) / 2.0
		role_info_popup.show()

func update_hud(_game: Node, player_id: int, allowed_actions: Array, active_dispatcher_target: int = -1) -> void:
	game = _game
	current_player = player_id
	var role = game.get_player_role(player_id)
	
	role_title.text = _get_role_name(role)
	
	if role in Globals.RoleColors:
		var style = StyleBoxFlat.new()
		var color = Globals.RoleColors[role]
		color.a = 0.8
		style.bg_color = color
		add_theme_stylebox_override("panel", style)

	event_selection.hide()

	if role == Globals.RoleType.Contingency:
		take_event_button.show()
		
		# Clear first
		available_take_actions.clear()
		for action in allowed_actions:
			var type = action & 0x1F # lower 5 bits for type
			if type == Globals.ActionType.PLANNER_TAKE:
				var card_id = (action >> 5) & 0x3F # target_city bits represent card_id here
				available_take_actions[card_id] = action

		# If you have available actions to take, the slot is definitively empty in engine context
		if not available_take_actions.is_empty():
			take_event_button.disabled = false
		else:
			# Even if no actions exist to take, disable the button if the slot isn't empty, or just broadly.
			take_event_button.disabled = true
	else:
		take_event_button.hide()

	if role == Globals.RoleType.Operations:
		expert_move_button.show()
		
		# Find expert moves
		available_expert_moves.clear()
		for action in allowed_actions:
			var type = action & 0x1F
			if type == Globals.ActionType.EXPERT_MOVE:
				var target_city = (action >> 5) & 0x3F
				var discard_city = (action >> 11) & 0x3F
				
				if not available_expert_moves.has(target_city):
					available_expert_moves[target_city] = {}
				available_expert_moves[target_city][discard_city] = action
				
		expert_move_button.disabled = available_expert_moves.is_empty()
		
		if available_expert_moves.is_empty() and expert_move_button.button_pressed:
			expert_move_button.button_pressed = false
			_on_expert_move_pressed()
	else:
		expert_move_button.hide()
		expert_move_button.button_pressed = false
		if is_expert_move_active:
			is_expert_move_active = false
			expert_move_toggled.emit(false)

	if role == Globals.RoleType.Dispatcher:
		dispatcher_container.show()
		_update_dispatcher_buttons(active_dispatcher_target)
	else:
		dispatcher_container.hide()

func _update_dispatcher_buttons(active_target: int) -> void:
	for child in dispatcher_container.get_children():
		child.queue_free()
		
	for i in range(game.get_player_count()):
		var btn = Button.new()
		btn.text = "P" + str(i + 1)
		btn.custom_minimum_size = Vector2(30, 24)
		btn.focus_mode = Control.FOCUS_NONE
		btn.add_theme_font_size_override("font_size", 12)
		
		# style similar to card_hud
		var btn_role = game.get_player_role(i)
		var style = StyleBoxFlat.new()
		if btn_role in Globals.RoleColors:
			var color = Globals.RoleColors[btn_role]
			if i == active_target:
				style.bg_color = color.lightened(0.2)
				style.set_border_width_all(2)
				style.border_color = Color.WHITE
			else:
				style.bg_color = color.darkened(0.4)
			style.set_corner_radius_all(4)
			btn.add_theme_stylebox_override("normal", style)
		
		btn.pressed.connect(func(): dispatcher_target_selected.emit(i))
		dispatcher_container.add_child(btn)

func _on_take_event_pressed() -> void:
	_populate_event_selection()
	_center_event_selection()
	event_selection.show()

func _center_event_selection() -> void:
	# Force center on screen since top_level = true can sometimes lose its anchor
	var screen_size = get_viewport_rect().size
	event_selection.global_position = (screen_size - event_selection.size) / 2.0

func _on_expert_move_pressed() -> void:
	is_expert_move_active = expert_move_button.button_pressed
	expert_move_toggled.emit(is_expert_move_active)

func show_expert_move_discard(target_city: int) -> void:
	if not available_expert_moves.has(target_city):
		return
		
	# Populate discard options
	for child in events_container.get_children():
		child.queue_free()
		
	selection_label.text = "Discard Card to Move"
	
	var discard_cards = available_expert_moves[target_city]
	for card_id in discard_cards.keys():
		var btn = Button.new()
		if game.has_method("get_card_name"):
			btn.text = game.get_card_name(card_id)
		else:
			btn.text = "Card " + str(card_id)
			
		btn.pressed.connect(func(): _on_expert_discard_selected(target_city, card_id))
		events_container.add_child(btn)
		
	_center_event_selection()
	event_selection.show()

func _on_expert_discard_selected(target_city: int, card_id: int) -> void:
	event_selection.hide()
	expert_move_button.button_pressed = false
	_on_expert_move_pressed()
	var raw_action = available_expert_moves[target_city][card_id]
	action_requested.emit(raw_action)

func _on_cancel_pressed() -> void:
	event_selection.hide()

func _populate_event_selection() -> void:
	selection_label.text = "Select Event Card"
	for child in events_container.get_children():
		child.queue_free()
		
	for card_id in available_take_actions.keys():
		var btn = Button.new()
		if game.has_method("get_card_name"):
			btn.text = game.get_card_name(card_id)
		else:
			btn.text = "Event Card " + str(card_id)
			
		btn.pressed.connect(func(): _on_event_selected(card_id))
		events_container.add_child(btn)

func _on_event_selected(card_id: int) -> void:
	event_selection.hide()
	var raw_action = available_take_actions[card_id]
	action_requested.emit(raw_action)

func _get_role_name(role: int) -> String:
	match role:
		Globals.RoleType.Contingency: return "Contingency Planner"
		Globals.RoleType.Dispatcher: return "Dispatcher"
		Globals.RoleType.Medic: return "Medic"
		Globals.RoleType.Operations: return "Operations Expert"
		Globals.RoleType.Quarantine: return "Quarantine Specialist"
		Globals.RoleType.Researcher: return "Researcher"
		Globals.RoleType.Scientist: return "Scientist"
		_: return "Unknown Role"
