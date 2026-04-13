extends PanelContainer

signal player_selected(player_id: int)
signal canceled()

@onready var header = $VBox/Header
@onready var container = $VBox/HBoxContainer
@onready var cancel_btn = $VBox/CancelButton

var _game: Node

func _ready():
	cancel_btn.pressed.connect(_on_cancel)
	hide()

func open(title: String, game: Node):
	header.text = title
	_game = game
	
	for child in container.get_children():
		child.queue_free()
		
	var pc = _game.get_player_count()
	for i in range(pc):
		var btn = Button.new()
		btn.text = "P" + str(i + 1)
		btn.custom_minimum_size = Vector2(40, 30)
		btn.add_theme_font_size_override("font_size", 14)
		
		var role = _game.get_player_role(i)
		if role in Globals.RoleColors:
			var color = Globals.RoleColors[role]
			var style = StyleBoxFlat.new()
			style.bg_color = color.darkened(0.4)
			style.set_corner_radius_all(4)
			btn.add_theme_stylebox_override("normal", style)
			
			var hover = style.duplicate()
			hover.bg_color = color.lightened(0.1)
			btn.add_theme_stylebox_override("hover", hover)
			
			var pressed = style.duplicate()
			pressed.bg_color = color.darkened(0.1)
			btn.add_theme_stylebox_override("pressed", pressed)
			
		btn.pressed.connect(func(): _on_player_selected(i))
		container.add_child(btn)
		
	_center_on_screen()
	show()

func _on_player_selected(pid: int):
	hide()
	player_selected.emit(pid)

func _on_cancel():
	hide()
	canceled.emit()

func _center_on_screen():
	var s = get_viewport_rect().size
	global_position = (s - size) / 2.0