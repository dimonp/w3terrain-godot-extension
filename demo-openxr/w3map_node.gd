@tool
extends W3MapNode

signal selector_changed(coords: Vector2i)

@onready var collision_node = $StaticBody3D
var selected_coords

func _unhandled_input(event):
	if event is InputEventMouseButton:
		if event.button_index == MouseButton.MOUSE_BUTTON_LEFT and event.pressed:
			var cell_coords = map.pick_cell_by_screen_position(event.position)
			if cell_coords is Vector2i:
				selected_coords = cell_coords
				selector_changed.emit(cell_coords)

func _ready():
	print("Map node ready: ", name)

func _on_map_initialized(_map_node: Object) -> void:
	var map_name = map.get_map_name()
	var map_size = map.get_map_size()
	print("Map '%s' : %s initilized." % [map_name, map_size])
	
func _on_map_info_ui_button_pressed_create_map(id: int) -> void:
	var size := Vector2i(33, 33)
	editor.create_map(size)
