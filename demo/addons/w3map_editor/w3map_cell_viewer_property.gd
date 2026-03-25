@tool
extends EditorProperty

var ui_scene = preload("res://addons/w3map_editor/cell_viewer_ui.tscn")
var ui_instance: Control

func _init():
	ui_instance = ui_scene.instantiate()
	add_child(ui_instance)
	set_bottom_editor(ui_instance) 

func _update_property():
	var property = get_edited_property()
	ui_instance.cell = get_edited_object()[property]
