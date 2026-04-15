extends PanelContainer

signal action_requested(action: int)

@onready var main_grid = $MarginContainer/VBoxContainer/Actions
@onready var color_picker = $MarginContainer/VBoxContainer/ColorPicker
@onready var title = $MarginContainer/VBoxContainer/Header/CityNameLabel

var active_city_id: int = -1

var available_actions = {}

func _init():
	for key in Globals.ActionType.values():
		available_actions[key] = {}

func _ready():
	# Ensure all basic buttons exist, or add them dynamically for missing types like Dispatcher moves
	var existing_types = []
	for btn in main_grid.get_children():
		existing_types.append(btn.get_meta("action_type"))
	
	for type in [Globals.ActionType.DISPATCHER_MOVE, Globals.ActionType.DISPATCHER_MOVE_AS]:
		if not existing_types.has(type):
			var btn = Button.new()
			btn.name = "DispatcherBtn_" + str(type)
			if type == Globals.ActionType.DISPATCHER_MOVE:
				btn.text = "Move To Pawn"
			elif type == Globals.ActionType.DISPATCHER_MOVE_AS:
				btn.text = "Move As Player"
			btn.set_meta("action_type", type)
			btn.layout_mode = 2
			btn.size_flags_horizontal = Control.SIZE_EXPAND_FILL
			main_grid.add_child(btn)

	# Connect all main buttons to one function using a loop
	for btn in main_grid.get_children():
		btn.pressed.connect(_on_action_pressed.bind(btn))
	
	# Connect color buttons
	$MarginContainer/VBoxContainer/ColorPicker/BlueBtn.pressed.connect(_on_color_selected.bind(Globals.CityColor.BLUE))
	$MarginContainer/VBoxContainer/ColorPicker/YellowBtn.pressed.connect(_on_color_selected.bind(Globals.CityColor.YELLOW))
	$MarginContainer/VBoxContainer/ColorPicker/BlackBtn.pressed.connect(_on_color_selected.bind(Globals.CityColor.BLACK))
	$MarginContainer/VBoxContainer/ColorPicker/RedBtn.pressed.connect(_on_color_selected.bind(Globals.CityColor.RED))

	# Add a close button
	var close_btn = Button.new()
	close_btn.text = "X"
	close_btn.custom_minimum_size = Vector2(24, 24)
	close_btn.pressed.connect(func(): hide())
	$MarginContainer/VBoxContainer/Header.add_child(close_btn)
	
	# Style the buttons
	for parent_grid in [main_grid, color_picker]:
		for btn in parent_grid.get_children():
			btn.add_theme_font_size_override("font_size", 14)
			
			# Color buttons specific styling
			if parent_grid == color_picker:
				var c_id = btn.get_meta("color_id")
				var btn_style = StyleBoxFlat.new()
				btn_style.set_corner_radius_all(4)
				
				var normal_color = Color.GRAY
				match c_id:
					Globals.CityColor.BLUE: normal_color = Color.DODGER_BLUE
					Globals.CityColor.YELLOW: normal_color = Color.GOLD
					Globals.CityColor.BLACK: normal_color = Color.DARK_SLATE_GRAY
					Globals.CityColor.RED: normal_color = Color.CRIMSON
					
				btn_style.bg_color = normal_color.darkened(0.2)
				btn.add_theme_stylebox_override("normal", btn_style)
				
				var hover_style = btn_style.duplicate()
				hover_style.bg_color = normal_color.lightened(0.1)
				btn.add_theme_stylebox_override("hover", hover_style)
				
				var pressed_style = btn_style.duplicate()
				pressed_style.bg_color = normal_color.darkened(0.4)
				btn.add_theme_stylebox_override("pressed", pressed_style)

func open(city_id: int, c_name: String, color_enum: int = 0):
	active_city_id = city_id
	title.text = c_name
	
	var style = StyleBoxFlat.new()
	var bg_color = Globals.get_city_color(color_enum).lerp(Color(0.1, 0.1, 0.1, 1.0), 0.7)
	bg_color.a = 0.8
	style.bg_color = bg_color
	add_theme_stylebox_override("panel", style)
	
	main_grid.show()
	color_picker.hide()
	
	# Enable/Disable buttons based on bridge data
	for btn in main_grid.get_children():
		var type = btn.get_meta("action_type")
		btn.hide()

		if available_actions.has(type) and available_actions[type].has(active_city_id):
			btn.show()

	for btn in color_picker.get_children():
		var color_id = btn.get_meta("color_id")
		btn.hide()
		if available_actions.has(Globals.ActionType.TREAT) and available_actions[Globals.ActionType.TREAT].has(active_city_id):
			if available_actions[Globals.ActionType.TREAT][active_city_id].has(color_id):
				btn.show()
	
	show()

func _on_action_pressed(btn: Button):
	var type = btn.get_meta("action_type")
	if type == Globals.ActionType.TREAT:
		if available_actions[type][active_city_id].size() == 1:
			var action : int = available_actions[type][active_city_id].values()[0]
			action_requested.emit(action)
		else:
			main_grid.hide()
			color_picker.show()
	else:
		var action : int = available_actions[type][active_city_id]
		action_requested.emit(action)
	hide()

func _on_color_selected(color_id: int):
	var action : int = available_actions[Globals.ActionType.TREAT][active_city_id][color_id]
	action_requested.emit(action)
	hide()

func reset_actions():
	for type in available_actions:
		available_actions[type].clear()
