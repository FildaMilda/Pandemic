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
@onready var station_icon = $StationIcon

@export var blue_icon: Texture2D
@export var yellow_icon: Texture2D
@export var black_icon: Texture2D
@export var red_icon: Texture2D

signal clicked(id: int, city_name: String)

var highlight: bool = false:
	set(value):
		highlight = value
		_update_highlight()

var highlight_color: Color = Color(0.1, 0.8, 0.3) # Default safe green

var highlight_pulse: float = 0.0

var has_station: bool = false:
	set(value):
		has_station = value
		if station_icon:
			station_icon.visible = has_station

func _process(delta):
	if highlight:
		highlight_pulse += delta * 3.5 # Slower pulse speed
		queue_redraw()

func _draw():
	if highlight:
		# Smaller pulsing radius and more subtle transparency
		var alpha = (sin(highlight_pulse) + 1.0) / 4.0 + 0.3 # Pulses between 0.3 and 0.8
		var radius = 17.5 + (sin(highlight_pulse) * 1.5) # Pulses tightly between 16 and 19
		
		# Draw a glowing aura and ring behind the city
		draw_circle(sprite.position, radius, Color(highlight_color, alpha * 0.25))
		draw_arc(sprite.position, radius, 0, TAU, 32, Color(highlight_color, alpha), 2.0, true)

func _ready():
	name_label.text = city_name
	_update_visuals()
	self.input_event.connect(_on_input_event)
	
	# Automatically move the station icon next to the text
	var font = name_label.get_theme_font("font")
	var font_size = name_label.get_theme_font_size("font_size")
	if font:
		var text_width = font.get_string_size(city_name, HORIZONTAL_ALIGNMENT_CENTER, -1, font_size).x
		if station_icon:
			# Shift it to the left of the text bounds, minus a small padding
			station_icon.position.x = -(text_width / 2.0) - 8.0

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
	if not highlight:
		highlight_pulse = 0.0
	queue_redraw()
