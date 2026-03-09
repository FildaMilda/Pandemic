extends Area2D

@export var city_name: String = "Default City"
@export var city_id: int = 0
@export var color_type: Globals.CityColor = Globals.CityColor.BLUE

@onready var name_label = $Label
@onready var sprite = $Sprite2D

@onready var red_counter = $DiseaseCounters/Red
@onready var blue_counter = $DiseaseCounters/Blue
@onready var black_counter = $DiseaseCounters/Black
@onready var yellow_counter = $DiseaseCounters/Yellow

@export var blue_icon: Texture2D
@export var yellow_icon: Texture2D
@export var black_icon: Texture2D
@export var red_icon: Texture2D

signal clicked(id: int, city_name: String)

var highlight: bool = false:
	set(value):
		highlight = value
		_update_highlight()

func _ready():
	name_label.text = city_name
	_update_visuals()
	self.input_event.connect(_on_input_event)

func set_counter(color : Globals.CityColor, value : int):
	if color == Globals.CityColor.BLUE:
		blue_counter.count = value
	elif color == Globals.CityColor.BLACK:
		black_counter.count = value
	elif color == Globals.CityColor.RED:
		red_counter.count = value
	elif color == Globals.CityColor.YELLOW:
		yellow_counter.count = value

func _on_input_event(_viewport, event, _shape_idx):
	if event is InputEventMouseButton and event.pressed and event.button_index == MOUSE_BUTTON_LEFT:
		_on_city_clicked()

func _on_city_clicked():
	clicked.emit(city_id, city_name)

func _update_visuals():
	var s = get_node_or_null("Sprite2D")
	if s:
		match color_type:
			Globals.CityColor.BLUE:
				s.texture = blue_icon
			Globals.CityColor.YELLOW:
				s.texture = yellow_icon
			Globals.CityColor.BLACK:
				s.texture = black_icon
			Globals.CityColor.RED:
				s.texture = red_icon
				
func _update_highlight():
	if highlight:
		sprite.self_modulate = Color(1.5, 1.5, 1.5, 1.0) 
	else:
		sprite.self_modulate = Color.WHITE
