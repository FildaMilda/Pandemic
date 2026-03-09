extends CanvasLayer

@export var card_scene: PackedScene = preload("res://scenes/Card.tscn")
@onready var container = $HandUI/CardContainer

func update_hand(game: Node):
	# 1. Clear current cards
	for child in container.get_children():
		child.queue_free()
	
	# 2. Get data from C++
	var current_p = game.get_current_player()
	var hand_ids = game.get_player_hand(current_p)
	
	# 3. Create new cards
	for card_id in hand_ids:
		var type = game.get_card_type(card_id)
		var name = game.get_card_name(card_id)
		var color = game.get_card_color(card_id)
		
		var new_card = card_scene.instantiate()
		container.add_child(new_card)
		
		# Convert your Globals.CityColor to an actual Godot Color
		var visual_color = _get_color_from_enum(type, color)
		new_card.setup(name, color)

func _get_color_from_enum(type, color_enum) -> Color:
	if type == Globals.CardType.EVENT:
		return Color.BURLYWOOD
	
	match color_enum:
		Globals.CityColor.BLUE: return Color.DODGER_BLUE
		Globals.CityColor.RED: return Color.CRIMSON
		Globals.CityColor.YELLOW: return Color.GOLD
		Globals.CityColor.BLACK: return Color.DARK_SLATE_GRAY
		_: return Color.GRAY
