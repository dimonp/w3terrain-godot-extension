extends Control

@onready var layer_inc_btn = $FoldableContainer/MarginContainer/VBoxContainer/HBoxContainerCellHeightLayer/ButtonInc
@onready var layer_dec_btn = $FoldableContainer/MarginContainer/VBoxContainer/HBoxContainerCellHeightLayer/ButtonDec

@onready var height_inc_btn = $FoldableContainer/MarginContainer/VBoxContainer/HBoxContainerCellGroundHeight/ButtonInc
@onready var height_dec_btn = $FoldableContainer/MarginContainer/VBoxContainer/HBoxContainerCellGroundHeight/ButtonDec

signal layer_increase_button_pressed()
signal layer_decrease_button_pressed()
signal height_increase_button_pressed()
signal height_decrease_button_pressed()

func _ready():
	layer_inc_btn.pressed.connect(func():
		accept_event()
		layer_increase_button_pressed.emit()
	)

	layer_dec_btn.pressed.connect(func():
		accept_event()
		layer_decrease_button_pressed.emit()
	)

	height_inc_btn.pressed.connect(func():
		accept_event()
		height_increase_button_pressed.emit()
	)

	height_dec_btn.pressed.connect(func():
		accept_event()
		height_decrease_button_pressed.emit()
	)
