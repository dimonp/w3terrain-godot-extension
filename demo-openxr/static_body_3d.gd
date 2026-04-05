extends StaticBody3D

@onready var collision_shape: CollisionShape3D = $CollisionShape3D

var _map
const CELL_SIZE = 128.0
enum CellTypes {
	GROUND	= 1 << 0,
	CLIFF	= 1 << 1,
	RAMP	= 1 << 2,
	WATER 	= 1 << 3
}

func _on_w_3_map_node_map_initialized(map_node: Object) -> void:
	_map = map_node.map
	collision_shape.shape = generate_terrain_collision()
	collision_shape.owner = null
	print("Collision shape generated.")

# Create a new HeightMapShape3D resource
func generate_terrain_collision() -> HeightMapShape3D:
	var size_2d = _map.get_map_size()

	var position_x = CELL_SIZE * (size_2d.x / 2) + CELL_SIZE / 2.0
	var position_z = -CELL_SIZE * (size_2d.y / 2) - CELL_SIZE / 2.0
	var node_transform = Transform3D() \
		.scaled(Vector3(CELL_SIZE, CELL_SIZE, CELL_SIZE)) \
		.translated(Vector3(position_x, 10.0, position_z))

	set_transform(node_transform)
	print("Height shape transform: ", node_transform)

	var heightmap_shape = HeightMapShape3D.new()
	heightmap_shape.map_width = size_2d.x
	heightmap_shape.map_depth = size_2d.y

	var map_data: PackedFloat32Array = generate_height_data(size_2d)
	print("Height maps size: ", map_data.size())

	heightmap_shape.map_data = map_data
	return heightmap_shape


func is_ground(type: int) -> bool:
	return type is int and type & CellTypes.GROUND

func is_water(type: int) -> bool:
	return type is int and type & CellTypes.WATER

func is_cliff(type: int) -> bool:
	return type is int and type & CellTypes.CLIFF

func is_coord_valid(x: int, y: int, size: Vector2i):
	return x < size.x and y < size.y

func set_height(data: PackedFloat32Array, x: int, y: int, size: Vector2i):
	if is_coord_valid(x, y, size):
		var coord: Vector2i = Vector2i(x, y)
		var cell_height = _map.get_cell_height(coord)
		# z is mirrored
		data[(size.y - y - 1) * size.x + x] = cell_height / CELL_SIZE

func generate_height_data(size: Vector2i) -> PackedFloat32Array:
	var data: PackedFloat32Array = PackedFloat32Array()
	data.resize(size.x * size.y)
	data.fill(NAN)
	var padding: int = (size.x / 100) * 5
	for y in range(size.y):
		for x in range(size.x):
			if (x < padding or x > size.x - padding): continue
			if (y < padding or y > size.y - padding): continue

			var coord: Vector2i = Vector2i(x, y)
			var cell_type = _map.get_cellpoint_type(coord)
			if is_water(cell_type): continue

			if is_ground(cell_type):
				set_height(data, x, y, size)

	return data
