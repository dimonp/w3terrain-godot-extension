extends EditorInspectorPlugin

const W3MapPropertyViewer = preload("res://addons/w3map_editor/w3map_cell_viewer_property.gd")
const W3MapPropertyEditor = preload("res://addons/w3map_editor/w3map_cell_editor_property.gd")

func _can_handle(object: Object) -> bool:
	return object is W3MapNode

func _parse_property(object: Object, type: Variant.Type, name: String, hint_type: PropertyHint, hint_string: String, usage_flags: int, wide: bool) -> bool:
	if name == "cell_viewer":
		add_property_editor(name, W3MapPropertyViewer.new())
		return true

	if name == "cell_editor":
		add_property_editor(name, W3MapPropertyEditor.new())
		return true

	return false
