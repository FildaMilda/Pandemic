extends PanelContainer

@onready var title_label = $VBoxContainer/Label
@onready var color_rect = $VBoxContainer/ColorRect

func setup(card_name: String, card_color: Color):
	title_label.text = card_name
	
	# Create a unique StyleBox to color the card background or header
	var style = get_theme_stylebox("panel").duplicate()
	style.bg_color = card_color.darkened(0.5) # Darken background slightly
	add_theme_stylebox_override("panel", style)
	
	color_rect.color = card_color
