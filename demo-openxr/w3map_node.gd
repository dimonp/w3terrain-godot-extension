@tool
extends W3MapNode

signal selector_changed(coords: Vector2i)

@onready var collision_node = $StaticBody3D
var selected_coords

func _ready():
	print("Map node ready: ", name)

func _on_map_initialized(_map_node: Object) -> void:
	var map_name = map.get_map_name()
	var map_size = map.get_map_size()
	print("Map '%s' : %s initilized." % [map_name, map_size])
	
func _on_map_info_ui_button_pressed_create_map(id: int) -> void:
	var size := Vector2i(33, 33)
	editor.create_map(size)

func update_selector(ray_origin: Vector3, ray_direction: Vector3):
	print("ray_origin: %s, ray_direction: %s" % [ray_origin, ray_direction])
	var cell_coords = map.pick_cell_by_ray(ray_origin, ray_direction)
	print("cell_coords: %s" % [cell_coords])
	if cell_coords is Vector2i:
		selected_coords = cell_coords
		selector_changed.emit(cell_coords)
		
func cell_bbox(coords: Vector2i):
	return map.get_cell_bbox(coords)
