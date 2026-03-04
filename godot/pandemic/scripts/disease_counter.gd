extends Node2D

@onready var label = $Label

func setup(color: Color, count: int):
	self.visible = (count > 0)
	label.text = str(count)
	queue_redraw() # Tells Godot to call _draw()

func _draw():
	# Draws a circle at (0,0) with radius 15
	# No texture required!
	draw_circle(Vector2.ZERO, 15, Color.BLACK) # Outline/Shadow
	draw_circle(Vector2.ZERO, 13, self.modulate) 

# In the inspector, set the Label's Horizontal and Vertical 
# Alignment to "Center" and move its position to (-15, -15) 
# to center it over the (0,0) draw point.
