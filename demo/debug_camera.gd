@tool
extends Node3D

func _process(_delta: float) -> void:
	if Engine.has_singleton("DebugDraw3D") and is_visible_in_tree() and Engine.is_editor_hint():
		var camera = self.get_parent()
		#DebugDraw3D.new_scoped_config() \
			#.set_viewport(get_viewport()) \
			#.set_thickness(5.0)
		DebugDraw3D.draw_camera_frustum(camera, Color.DARK_ORANGE)
		#DebugDraw3D.draw_camera_frustum_planes(camera.get_frustum(), Color.DARK_ORANGE)
