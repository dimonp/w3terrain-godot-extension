@tool
extends EditorPlugin

var gizmo_plugin = preload("res://addons/w3map_editor/w3map_selector_gizmo.gd").new()
var inspector_plugin = preload("res://addons/w3map_editor/w3map_editor_inspector.gd").new()

var edited_object: W3MapNode = null

func _enter_tree():
	add_inspector_plugin(inspector_plugin)
	add_node_3d_gizmo_plugin(gizmo_plugin)
	
func _exit_tree():
	remove_node_3d_gizmo_plugin(gizmo_plugin)
	remove_inspector_plugin(inspector_plugin)

func _handles(object: Object) -> bool:
	return object is W3MapNodeEditor

func _edit(object: Object) -> void:
	edited_object = object as W3MapNodeEditor

func _make_visible(visible: bool) -> void:
	if not visible:
		edited_object = null

func _forward_3d_gui_input(viewport_camera: Camera3D, event: InputEvent) -> int:
	if event is InputEventMouseButton:
		if event.button_index == MOUSE_BUTTON_LEFT and event.pressed:
			var ray_origin = viewport_camera.project_ray_origin(event.position)
			var ray_direction = viewport_camera.project_ray_normal(event.position)
			var cell_coords = edited_object.map.pick_cell_by_ray(ray_origin, ray_direction)
			if cell_coords is Vector2i:
				edited_object.selected_coords = cell_coords
			return EditorPlugin.AFTER_GUI_INPUT_STOP
			
	if event is InputEventKey:
		if event.keycode == KEY_SPACE:
			if event.pressed:
				edited_object.do_action()
				return EditorPlugin.AFTER_GUI_INPUT_STOP

	return EditorPlugin.AFTER_GUI_INPUT_PASS
