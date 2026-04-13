extends Camera2D

@export var zoom_speed: float = 0.05
@export var min_zoom: float = 0.1
@export var max_zoom: float = 4.0

@onready var map_sprite = $"../Map"

var is_panning: bool = false

func _ready() -> void:
	_calculate_min_zoom()

func _input(event: InputEvent) -> void:
	# --- ZOOMING ---
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_WHEEL_DOWN:
			_zoom_camera(-zoom_speed)
		elif event.button_index == MOUSE_BUTTON_WHEEL_UP:
			_zoom_camera(zoom_speed)
			
		# --- START/STOP PANNING ---
		if event.button_index == MOUSE_BUTTON_MIDDLE:
			is_panning = event.pressed

	# --- PANNING MOVEMENT ---
	if event is InputEventMouseMotion and is_panning:
		# 1. Calculate the intended new position
		var target_pos = position - (event.relative) / zoom.x
		
		# 2. Get the screen size so we don't stop at the center, but at the edge
		var view_size = get_viewport_rect().size / zoom.x
		
		target_pos.x = clamp(target_pos.x, limit_left + view_size.x/2, limit_right - view_size.x/2)
		target_pos.y = clamp(target_pos.y, limit_top + view_size.y/2, limit_bottom - view_size.y/2)
		
		# 4. Apply the clamped position
		position = target_pos

func _zoom_camera(delta: float):
	var new_zoom = clamp(zoom.x + delta, min_zoom, max_zoom)
	zoom = Vector2(new_zoom, new_zoom)

func _calculate_min_zoom():
	var screen_size = get_viewport_rect().size
	var map_size = map_sprite.texture.get_size() * map_sprite.scale
	
	# Choose the larger ratio to ensure the map always fills the screen
	var min_zoom_x = screen_size.x / map_size.x
	var min_zoom_y = screen_size.y / map_size.y
	min_zoom = max(min_zoom_x, min_zoom_y)

func focus_on_position(target_pos: Vector2, target_zoom: float = 1.6):
	target_zoom = clamp(target_zoom, min_zoom, max_zoom)
	var view_size = get_viewport_rect().size / target_zoom
	
	target_pos.x = clamp(target_pos.x, limit_left + view_size.x/2, limit_right - view_size.x/2)
	target_pos.y = clamp(target_pos.y, limit_top + view_size.y/2, limit_bottom - view_size.y/2)
	
	var tween = create_tween().set_parallel(true)
	tween.tween_property(self, "position", target_pos, 0.6).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
	tween.tween_property(self, "zoom", Vector2(target_zoom, target_zoom), 0.6).set_trans(Tween.TRANS_SINE).set_ease(Tween.EASE_OUT)
