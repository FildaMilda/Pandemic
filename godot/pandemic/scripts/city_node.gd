extends Area2D

enum CityColor {BLUE, YELLOW, BLACK, RED}

@export var city_name: String = "Default City"
@export var city_id: int = 0
@export var color_type: CityColor = CityColor.BLUE

@onready var name_label = $Label
@onready var sprite = $Sprite2D

@export var blue_icon: Texture2D
@export var yellow_icon: Texture2D
@export var black_icon: Texture2D
@export var red_icon: Texture2D

func _ready():
	name_label.text = city_name
	_update_visuals()
	self.input_event.connect(_on_input_event)

func _on_input_event(_viewport, event, _shape_idx):
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_on_city_clicked()

func _on_city_clicked():
	print("Clicked: ", city_name, " (ID: ", city_id, ")")
	
	# This is where you talk to your C++ Logic
	# Example: PandemicGame.on_city_selected(city_id)
	
	# Visual feedback: Let's pulse the city when clicked
	var tween = create_tween()
	tween.tween_property(sprite, "scale", Vector2(1.5, 1.5), 0.1)
	tween.tween_property(sprite, "scale", Vector2(1.0, 1.0), 0.1)

func _update_visuals():
	var s = get_node_or_null("Sprite2D")
	if s:
		match color_type:
			CityColor.BLUE:
				s.texture = blue_icon
			CityColor.YELLOW:
				s.texture = yellow_icon
			CityColor.BLACK:
				s.texture = black_icon
			CityColor.RED:
				s.texture = red_icon
