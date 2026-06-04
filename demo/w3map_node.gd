@tool
extends W3MapNodeEditor

signal selector_changed(coords: Vector2i)

@onready var collision_node = $StaticBody3D

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
	print("Map '%s' initilized." % [map_name])

	var map_info_ui = $"%MapInfoUI"
	map_info_ui.map_name = map_name
	map_info_ui.map_size = map_size

func _on_map_info_ui_create_new_map(size_x: int, size_y: int) -> void:
	var size := Vector2i(size_x, size_y)
	editor.create_map(size, 1, 1)

func _on_map_initialization_progress(map_node: Object, percent: int) -> void:
	var pb = %ProgressBarMapLoad
	pb.set_anchors_and_offsets_preset(Control.PRESET_CENTER)
	pb.value = percent;
	if percent < 100:
		pb.visible = true
	else:
		pb.visible = false

func _on_map_info_ui_toggle_normals() -> void:
	var terrain_surface = $W3SurfaceTerrain
	terrain_surface.render_debug = !terrain_surface.render_debug
