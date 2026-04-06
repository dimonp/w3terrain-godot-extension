@tool
extends W3MapNode

signal selector_changed(coords: Vector2i)

@onready var collision_node = $StaticBody3D
var selected_coords: Vector2i
var selected_bbox: AABB

const CELL_INFO_PANEL = preload("res://floating_info.tscn")
var info_panel

func _ready():
	print("Map node ready: ", name)
	info_panel = CELL_INFO_PANEL.instantiate()
	add_child(info_panel)

func _on_map_initialized(_map_node: Object) -> void:
	var map_name = map.get_map_name()
	var map_size = map.get_map_size()
	print("Map '%s' : %s initilized." % [map_name, map_size])
	
func _on_map_info_ui_button_pressed_create_map(id: int) -> void:
	var size := Vector2i(33, 33)
	editor.create_map(size)

func update_selector(ray_origin: Vector3, ray_direction: Vector3):
	var cell_coords = map.pick_cell_by_ray(ray_origin, ray_direction)
	if cell_coords is Vector2i:
		selected_coords = cell_coords
		selected_bbox = map.get_cell_bbox(selected_coords)
		selector_changed.emit(cell_coords)
		update_cell_ui()

func selected_cell_bbox():
	return selected_bbox

func update_cell_ui():
	info_panel.cell_coord = selected_coords
	info_panel.cell_position = selected_bbox.get_center()
	info_panel.cell_layer = map.get_cellpoint_layer(selected_coords)
	info_panel.cell_tileset = Vector2i (
		map.get_cellpoint_ground_tileset(selected_coords),
		map.get_cellpoint_geo_tileset(selected_coords))
	info_panel.cell_type = map.get_cellpoint_type(selected_coords)
	
	var pos = selected_bbox.get_center()
	pos.y = selected_bbox.end.y + 100.0
	info_panel.position = pos
	
