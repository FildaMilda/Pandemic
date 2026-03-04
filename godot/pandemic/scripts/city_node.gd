extends Area2D

enum CityColor {BLUE, YELLOW, BLACK, RED}

@export var city_name: String = "Default City"
@export var city_id: int = 0
@export var color_type: CityColor = CityColor.BLUE

@onready var name_label = $Label
@onready var sprite = $Sprite2D
@onready var counter_container = $DiseaseCounters

@onready var counters = {
	CityColor.BLUE: $DiseaseCounters/BlueCounter,
	CityColor.YELLOW: $DiseaseCounters/YellowCounter,
	CityColor.BLACK: $DiseaseCounters/BlackCounter,
	CityColor.RED: $DiseaseCounters/RedCounter
}

@export var blue_icon: Texture2D
@export var yellow_icon: Texture2D
@export var black_icon: Texture2D
@export var red_icon: Texture2D

var disease_counts = {
	CityColor.BLUE: 0,
	CityColor.YELLOW: 0,
	CityColor.BLACK: 0,
	CityColor.RED: 0
}

func _ready():
	name_label.text = city_name
	_update_visuals()
	sync_with_cpp_logic()
	self.input_event.connect(_on_input_event)
	
func sync_with_cpp_logic():
	# Define colors for the draw function
	var color_map = {
		CityColor.BLUE: Color.CORNFLOWER_BLUE,
		CityColor.YELLOW: Color.GOLD,
		CityColor.BLACK: Color.DARK_SLATE_GRAY,
		CityColor.RED: Color.CRIMSON
	}

	for type in CityColor.values():
		# Using your C++ Bridge: PandemicGame.getDisease(city_id, type)
		var count = 1
		
		var counter_node = counters[type]
		counter_node.modulate = color_map[type]
		counter_node.setup(color_map[type], count)
	
func _setup_counters():
	# Set the colors for the counter sprites once
	counters[CityColor.BLUE].set_color(Color.MEDIUM_BLUE)
	counters[CityColor.YELLOW].set_color(Color.GOLD)
	counters[CityColor.BLACK].set_color(Color.BLACK)
	counters[CityColor.RED].set_color(Color.DARK_RED)
	
	# Initialize all to 0 (hidden)
	update_counter_display()

func update_counter_display():
	for color in disease_counts.keys():
		counters[color].set_count(disease_counts[color])

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
	print(s)
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
