extends PanelContainer

signal execute_event_requested(card_id: int)

@onready var header = $MarginContainer/VBox/Header
@onready var description = $MarginContainer/VBox/DescriptionLabel
@onready var use_btn = $MarginContainer/VBox/HBox/UseButton
@onready var cancel_btn = $MarginContainer/VBox/HBox/CancelButton

var _card_id: int = -1

func _ready() -> void:
	use_btn.pressed.connect(_on_use_pressed)
	cancel_btn.pressed.connect(hide)
	hide()

func open(card_id: int, card_name: String, desc_text: String) -> void:
	_card_id = card_id
	header.text = card_name
	description.text = desc_text
	
	show()

func _on_use_pressed() -> void:
	hide()
	execute_event_requested.emit(_card_id)
