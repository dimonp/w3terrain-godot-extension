extends W3MapNode

@onready var collision_node = $StaticBody3D

var cell_coords: Vector2i
var cell_info_ui

func _ready():
	print("Map node ready.")

func _on_map_initialized(_map_node: Object) -> void:
	var map_name = map.get_map_name()
	var map_size = map.get_map_size()
	
	print("Map '%s' initilized." % [map_name])
	cell_info_ui = $"%CellInfoUI"
	
	var map_info_ui = $"%MapInfoUI"
	#await map_info_ui.ready

	map_info_ui.map_name = map_name
	map_info_ui.map_size = map_size
			
func _on_w_3_map_cell_selector_selector_changed(coords: Vector2i) -> void:
	cell_coords = coords

	cell_info_ui.cell_coord = cell_coords

	var layer = map.get_cellpoint_layer(cell_coords)
	if layer is int:
		cell_info_ui.cell_layer = layer

	var cell_position = map.get_cellpoint_position(cell_coords)
	if cell_position is Vector3:
		cell_info_ui.cell_position = cell_position
	
	var type = map.get_cellpoint_type(cell_coords)
	if type is int:
		cell_info_ui.cell_type = type

	var ground_tileset = map.get_cellpoint_ground_tilset(cell_coords)
	var geo_tileset = map.get_cellpoint_geo_tilset(cell_coords)
	if ground_tileset is int and geo_tileset is int:
		cell_info_ui.cell_tileset = Vector2i(ground_tileset, geo_tileset)

func _on_editor_info_ui_layer_increase_button_pressed() -> void:
	editor.increase_cellpoint_height_layer(cell_coords)

func _on_editor_info_ui_layer_decrease_button_pressed() -> void:
	editor.decrease_cellpoint_height_layer(cell_coords)

func _on_editor_info_ui_height_increase_button_pressed() -> void:
	var cell_height = map.get_cellpoint_ground_height(cell_coords)
	if cell_height is float:
		editor.set_cellpoint_ground_height(cell_coords, cell_height + 5)

func _on_editor_info_ui_height_decrease_button_pressed() -> void:
	var cell_height = map.get_cellpoint_ground_height(cell_coords)
	if cell_height is float:
		editor.set_cellpoint_ground_height(cell_coords, cell_height - 5)


func _on_map_info_ui_button_pressed_create_map(id: int) -> void:
	var size := Vector2i(33, 33)
	editor.create_map(size)
