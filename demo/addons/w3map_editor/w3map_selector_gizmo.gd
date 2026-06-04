extends EditorNode3DGizmoPlugin

func _get_gizmo_name() -> String:
	return "W3MapSelectorGizmo"

func _has_gizmo(node: Node3D) -> bool:
	return node is W3MapNode
	
func _init():
	var mat = StandardMaterial3D.new()
	mat.vertex_color_use_as_albedo = true
	mat.shading_mode = BaseMaterial3D.SHADING_MODE_UNSHADED
	add_material("main", mat)

func _redraw(gizmo: EditorNode3DGizmo):
	var node = gizmo.get_node_3d()
	var bbox = node.brush_bbox()
	if bbox is AABB:
		update_mesh(bbox, gizmo)
	
func update_mesh(aabb: AABB, gizmo: EditorNode3DGizmo):
	gizmo.clear()

	var st = SurfaceTool.new()
	
	var min_pos = aabb.position
	var max_pos = aabb.position + aabb.size
	
	min_pos.y -= 10
	max_pos.y += 10
	
	# Define the 8 corners of the AABB
	var p0 = Vector3(min_pos.x, min_pos.y, min_pos.z)
	var p1 = Vector3(max_pos.x, min_pos.y, min_pos.z)
	var p2 = Vector3(max_pos.x, max_pos.y, min_pos.z)
	var p3 = Vector3(min_pos.x, max_pos.y, min_pos.z)
	var p4 = Vector3(min_pos.x, min_pos.y, max_pos.z)
	var p5 = Vector3(max_pos.x, min_pos.y, max_pos.z)
	var p6 = Vector3(max_pos.x, max_pos.y, max_pos.z)
	var p7 = Vector3(min_pos.x, max_pos.y, max_pos.z)
	
	st.begin(Mesh.PRIMITIVE_LINES)
	st.set_material(get_material("main", gizmo))
	
	# Define the 12 edges by adding pairs of vertices
	st.set_color(Color(1.0, 1.0, 0.0, 1.0))
	st.add_vertex(p0)
	st.add_vertex(p1) # Bottom front edge

	st.add_vertex(p1)
	st.add_vertex(p2) # Bottom right edge
	st.add_vertex(p2)
	st.add_vertex(p3) # Bottom back edge
	st.add_vertex(p3)
	st.add_vertex(p0) # Bottom left edge

	st.add_vertex(p4)
	st.add_vertex(p5) # Top front edge
	st.add_vertex(p5)
	st.add_vertex(p6) # Top right edge
	
	st.set_color(Color(1.0, 0.0, 0.0, 1.0))
	st.add_vertex(p6)
	st.add_vertex(p7) # Top back edge
	st.add_vertex(p7)
	st.add_vertex(p4) # Top left edge

	st.set_color(Color(1.0, 1.0, 0.0, 1.0))
	st.add_vertex(p0)
	st.add_vertex(p4) # Left front vertical edge
	st.add_vertex(p1)
	st.add_vertex(p5) # Right front vertical edge
	st.add_vertex(p2)
	st.add_vertex(p6) # Right back vertical edge
	
	st.set_color(Color(1.0, 0.0, 0.0, 1.0))
	st.add_vertex(p3)
	st.add_vertex(p7) # Left back vertical edge
	
	var mesh = st.commit()
	gizmo.add_mesh(mesh)	

func update_mesh_new(node: W3MapNode, gizmo: EditorNode3DGizmo):
	gizmo.clear()

	var st = SurfaceTool.new()
	
	var brush_size = node.brush_size
	var coords = node.selected_coords
	var map = node.w3e_map

	
	for y in range(brush_size):
		for x in range(brush_size):
			var curr_coords = Vector2i(
				coords.x + x - brush_size / 2, 
				coords.y + y - brush_size / 2,
			)

			var position = map.get_cellpoint_get_cellpoint_position(curr_coords)
	
	
	var mesh = st.commit()
	gizmo.add_mesh(mesh)	
