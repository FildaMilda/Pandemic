extends PanelContainer

signal action_requested(action: int)

@onready var main_grid = $VBoxContainer/Actions
@onready var color_picker = $VBoxContainer/ColorPicker
@onready var title = $VBoxContainer/CityNameLabel

var active_city_id: int = -1

var available_actions = {
	Globals.ActionType.DRIVE: {},
	Globals.ActionType.DIRECT_FLIGHT: {},
	Globals.ActionType.CHARTER_FLIGHT: {},
	Globals.ActionType.SHUTTLE_FLIGHT: {},
	Globals.ActionType.TREAT: {},
	Globals.ActionType.BUILD: {},
	Globals.ActionType.SHARE: {},
	Globals.ActionType.CURE: {}
}

func _ready():
	# Connect all main buttons to one function using a loop
	for btn in main_grid.get_children():
		btn.pressed.connect(_on_action_pressed.bind(btn))
	
	# Connect color buttons
	$VBoxContainer/ColorPicker/BlueBtn.pressed.connect(_on_color_selected.bind(Globals.CityColor.BLUE))
	$VBoxContainer/ColorPicker/YellowBtn.pressed.connect(_on_color_selected.bind(Globals.CityColor.YELLOW))
	$VBoxContainer/ColorPicker/BlackBtn.pressed.connect(_on_color_selected.bind(Globals.CityColor.BLACK))
	$VBoxContainer/ColorPicker/RedBtn.pressed.connect(_on_color_selected.bind(Globals.CityColor.RED))

func open(city_id: int, c_name: String):
	active_city_id = city_id
	title.text = c_name
	main_grid.show()
	color_picker.hide()
	
	# Enable/Disable buttons based on bridge data
	for btn in main_grid.get_children():
		var type = btn.get_meta("action_type")
		btn.disabled = true
		
		if available_actions[type].has(active_city_id):
			btn.disabled = false
	
	for btn in color_picker.get_children():
		var color_id = btn.get_meta("color_id")
		btn.disabled = true
		if available_actions[Globals.ActionType.TREAT].has(active_city_id):
			if available_actions[Globals.ActionType.TREAT][active_city_id].has(color_id):
				btn.disabled = false
	
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
