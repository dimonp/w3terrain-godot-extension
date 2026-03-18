@tool
extends MeshInstance3D

@export_range(1, 1000) var axis_size: float = 100.0
@export_range(0.01, 1) var axis_thickness: float = 1

func _validate_property(property: Dictionary):
	if property["name"] == "mesh":
		property["usage"] = PROPERTY_USAGE_NONE

func _process(_delta: float) -> void:
	if is_visible_in_tree():
		position = Vector3.ZERO
		scale = Vector3.ONE
		
		var material = StandardMaterial3D.new()
		material.shading_mode = StandardMaterial3D.SHADING_MODE_UNSHADED
		material.vertex_color_use_as_albedo = true
		set_material_override(material)
	
		var st = SurfaceTool.new()
		st.begin(Mesh.PRIMITIVE_TRIANGLES)
		# Function to add a thick cylinder (axis)
		# start: Vector3, end: Vector3, color: Color, radius: float
		create_cylinder_axis(st, Vector3.ZERO, Vector3(axis_size, 0, 0), Color.RED, axis_thickness)   # X-Axis
		create_cylinder_axis(st, Vector3.ZERO, Vector3(0, axis_size, 0), Color.GREEN, axis_thickness) # Y-Axis
		create_cylinder_axis(st, Vector3.ZERO, Vector3(0, 0, axis_size), Color.BLUE, axis_thickness)  # Z-Axis
		st.index()
		mesh = st.commit()
	
func create_cylinder_axis(st: SurfaceTool, pos_from: Vector3, pos_to: Vector3, color: Color, radius: float):
	var dir = (pos_to - pos_from).normalized()
	var right = Vector3.UP.cross(dir).normalized()
	if right.length() < 0.1: right = Vector3.RIGHT.cross(dir).normalized()
	var up = dir.cross(right).normalized()
	
	var sides = 3
	for i in range(sides):
		var ang1 = i * PI * 2 / sides
		var ang2 = (i + 1) * PI * 2 / sides
		
		var v1 = pos_from + (right * cos(ang1) + up * sin(ang1)) * radius
		var v2 = pos_from + (right * cos(ang2) + up * sin(ang2)) * radius
		var v3 = pos_to + (right * cos(ang2) + up * sin(ang2)) * radius
		var v4 = pos_to + (right * cos(ang1) + up * sin(ang1)) * radius
		
		# Add two triangles for each side segment
		st.set_color(color)
		st.add_vertex(v1)
		st.add_vertex(v3)
		st.add_vertex(v2)
		
		st.add_vertex(v1)
		st.add_vertex(v4)
		st.add_vertex(v3)
