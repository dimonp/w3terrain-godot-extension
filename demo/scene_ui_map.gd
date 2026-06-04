extends Control

signal toggle_normals()
signal create_new_map(size_x: int, size_y: int)

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

func _on_button_create_32_pressed() -> void:
	emit_signal("create_new_map", 33, 33)

func _on_button_create_1024_pressed() -> void:
	emit_signal("create_new_map", 1025, 1025)

func _on_button_toggle_normals_pressed() -> void:
	emit_signal("toggle_normals")
