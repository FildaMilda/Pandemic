extends Panel

@onready var label = $Label

@export var count: int = 0:
	set(value):
		count = value
		_update_ui()

func _ready():
	_update_ui()

func _update_ui():
	# 1. Update the text
	if label:
		label.text = str(count)
	
	# 2. Hide if zero, show if not
	if count <= 0:
		visible = false
	else:
		visible = true

# Function to change text from other scripts
func set_number(val: int):
	label.text = str(val)
