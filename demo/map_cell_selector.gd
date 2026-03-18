class_name W3MapCellSelector extends W3Surface

signal selector_changed(coords: Vector2i)

@export var shader_material: ShaderMaterial

var _map

func _ready():
	var _shader_material = ShaderMaterial.new()
	_map = map_node.map

func _unhandled_input(event):
	if event is InputEventMouseButton:
		if event.button_index == MouseButton.MOUSE_BUTTON_LEFT and event.pressed:
			var cell_coords = _map.pick_cell_by_screen_position(event.position)
			if cell_coords is Vector2i:
				selector_changed.emit(cell_coords)
			
				var aabb = _map.get_cell_bbox(cell_coords)
				if aabb is AABB:
					update_mesh(aabb);

func update_mesh(aabb: AABB):
	_mesh.clear_surfaces()
	
	var min_pos = aabb.position
	var max_pos = aabb.position + aabb.size
	
	min_pos.y -= 10
	max_pos.y += 10
	
	begin_render(true)

	_surface_tool.set_material(shader_material);
	
	# Define the 8 corners of the AABB
	var p0 = Vector3(min_pos.x, min_pos.y, min_pos.z)
	var p1 = Vector3(max_pos.x, min_pos.y, min_pos.z)
	var p2 = Vector3(max_pos.x, max_pos.y, min_pos.z)
	var p3 = Vector3(min_pos.x, max_pos.y, min_pos.z)
	var p4 = Vector3(min_pos.x, min_pos.y, max_pos.z)
	var p5 = Vector3(max_pos.x, min_pos.y, max_pos.z)
	var p6 = Vector3(max_pos.x, max_pos.y, max_pos.z)
	var p7 = Vector3(min_pos.x, max_pos.y, max_pos.z)
	
	# Define the 12 edges by adding pairs of vertices
	_surface_tool.set_color(Color(1.0, 1.0, 0.0, 1.0))
	_surface_tool.add_vertex(p0)
	_surface_tool.add_vertex(p1) # Bottom front edge

	_surface_tool.add_vertex(p1)
	_surface_tool.add_vertex(p2) # Bottom right edge
	_surface_tool.add_vertex(p2)
	_surface_tool.add_vertex(p3) # Bottom back edge
	_surface_tool.add_vertex(p3)
	_surface_tool.add_vertex(p0) # Bottom left edge

	_surface_tool.add_vertex(p4)
	_surface_tool.add_vertex(p5) # Top front edge
	_surface_tool.add_vertex(p5)
	_surface_tool.add_vertex(p6) # Top right edge
	
	_surface_tool.set_color(Color(1.0, 0.0, 0.0, 1.0))
	_surface_tool.add_vertex(p6)
	_surface_tool.add_vertex(p7) # Top back edge
	_surface_tool.add_vertex(p7)
	_surface_tool.add_vertex(p4) # Top left edge

	_surface_tool.set_color(Color(1.0, 1.0, 0.0, 1.0))
	_surface_tool.add_vertex(p0)
	_surface_tool.add_vertex(p4) # Left front vertical edge
	_surface_tool.add_vertex(p1)
	_surface_tool.add_vertex(p5) # Right front vertical edge
	_surface_tool.add_vertex(p2)
	_surface_tool.add_vertex(p6) # Right back vertical edge
	
	_surface_tool.set_color(Color(1.0, 0.0, 0.0, 1.0))
	_surface_tool.add_vertex(p3)
	_surface_tool.add_vertex(p7) # Left back vertical edge	
	
	end_render()
