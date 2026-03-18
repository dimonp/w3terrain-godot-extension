extends Control
signal button_pressed_create_map(id: int)

@onready var cell_name_ctrl = $FoldableContainer/MarginContainer/VBoxContainer/HBoxContainerCellName/Text
@onready var cell_size_ctrl = $FoldableContainer/MarginContainer/VBoxContainer/HBoxContainerCellSize/Text

@export var map_name: String:
	set(value):
		if cell_name_ctrl:
			cell_name_ctrl.text = str(value)

@export var map_size: Vector2i:
	set(value):
		if cell_size_ctrl:
			cell_size_ctrl.text = str(value)


func _on_button_pressed() -> void:
	emit_signal("button_pressed_create_map", 1)
