extends PanelContainer

@export var card_scene: PackedScene = preload("res://scenes/card.tscn")

@onready var outbreaks_label = $MarginContainer/HBoxContainer/LeftStats/OutbreakBox/OutbreaksLabel
@onready var infection_rate_label = $MarginContainer/HBoxContainer/LeftStats/InfectionBox/InfectionRateLabel

@onready var last_player_cards_btn = $MarginContainer/HBoxContainer/Menus/LastPlayerCardsBtn
@onready var last_infection_cards_btn = $MarginContainer/HBoxContainer/Menus/LastInfectionCardsBtn
@onready var player_discard_btn = $MarginContainer/HBoxContainer/Menus/PlayerDiscardBtn
@onready var infection_discard_btn = $MarginContainer/HBoxContainer/Menus/InfectionDiscardBtn

@onready var popup_panel = $CardPopup
@onready var popup_card_container = $CardPopup/MarginContainer/ScrollContainer/CardContainer

var _game: Node
var _last_player_cards: Array = []
var _last_infection_cards: Array = []
var _last_popup_hide_time: int = 0
var _current_popup_button: Control = null

func _ready():
	last_player_cards_btn.pressed.connect(_on_last_player_cards_pressed)
	last_infection_cards_btn.pressed.connect(_on_last_infection_cards_pressed)
	player_discard_btn.pressed.connect(_on_player_discard_pressed)
	infection_discard_btn.pressed.connect(_on_infection_discard_pressed)
	popup_panel.popup_hide.connect(_on_popup_hide)

func _on_popup_hide():
	_last_popup_hide_time = Time.get_ticks_msec()

func update_stats(game: Node):
	_game = game
	outbreaks_label.text = "Outbreaks: %d/8" % game.get_outbreak_count()
	infection_rate_label.text = "Infection Rate: %d" % game.get_infection_rate_amount()

func turn_finished(turn_info: Dictionary):
	if turn_info.has("turn_ended") and turn_info["turn_ended"]:
		if turn_info.has("drawn_player_cards"):
			_last_player_cards = turn_info["drawn_player_cards"]
		if turn_info.has("drawn_infection_cards"):
			_last_infection_cards = turn_info["drawn_infection_cards"]
	if _game != null:
		update_stats(_game)

func _on_last_player_cards_pressed():
	if _game:
		_show_cards(_last_player_cards, last_player_cards_btn)

func _on_last_infection_cards_pressed():
	if _game:
		_show_cards(_last_infection_cards, last_infection_cards_btn)

func _on_player_discard_pressed():
	if _game:
		_show_cards(_game.get_player_discard_pile(), player_discard_btn)

func _on_infection_discard_pressed():
	if _game:
		_show_cards(_game.get_infection_discard_pile(), infection_discard_btn)



func _show_cards(card_ids: Array, button: Control):
	if _current_popup_button == button and Time.get_ticks_msec() - _last_popup_hide_time < 100:
		return
	
	_current_popup_button = button
	for child in popup_card_container.get_children():
		child.queue_free()

	for card_id in card_ids:
		var type = _game.get_card_type(card_id)
		var name = _game.get_card_name(card_id)
		var color = _game.get_card_color(card_id)

		if type == Globals.CardType.EPIDEMIC:
			name = "Epidemic"

		var card_inst = card_scene.instantiate()
		popup_card_container.add_child(card_inst)
		card_inst.setup(card_id, name, type, color)

	var global_pos = button.global_position
	popup_panel.position = global_pos + Vector2(0, button.size.y)
	popup_panel.popup()
