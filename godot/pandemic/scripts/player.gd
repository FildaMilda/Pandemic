extends Node2D

@onready var panel = $Panel
@onready var label = $Panel/Label
var id : int 

var role_colors = {
	Globals.RoleType.Medic: Color.CORAL,
	Globals.RoleType.Scientist: Color.GHOST_WHITE,
	Globals.RoleType.Researcher: Color.SADDLE_BROWN,
	Globals.RoleType.Operations: Color.LIGHT_GREEN,
	Globals.RoleType.Dispatcher: Color.VIOLET,
	Globals.RoleType.Contingency: Color.AQUAMARINE,
	Globals.RoleType.Quarantine: Color.DARK_GREEN
}

func setup(player_id: int, role: Globals.RoleType) -> void:
	label.text = "P" + str(player_id + 1)
	id = player_id
	
	var new_style = panel.get_theme_stylebox("panel").duplicate()
	if role in role_colors:
		new_style.bg_color = role_colors[role]
	
	panel.add_theme_stylebox_override("panel", new_style)
