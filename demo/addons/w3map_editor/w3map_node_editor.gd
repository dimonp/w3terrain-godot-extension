extends W3MapNode
class_name W3MapNodeEditor

enum BrushAction { 
	GROUND_UP, 
	GROUND_DOWN,
	LAYER_UP, 
	LAYER_DOWN,
	RAMP_ADD, 
	RAMP_DEL,
}

var cell_viewer: Dictionary = {
	"coords"	: Vector2i(),
	"position"	: Vector3(),
	"layer"		: int(0),
	"height"	: float(0.0),
	"tilesets"	: Vector2i(),
	"types"		: int(0),
}

var cell_editor: Dictionary = {
	"brush_size"	: int(0),
	"brush_action"	: -1
}

var selected_coords: Vector2i:
	set(value):
		print("selected_coords ",value)
		selected_coords = value
		cell_viewer["coords"] = selected_coords
		cell_viewer["position"] = map.get_cellpoint_position(selected_coords)
		cell_viewer["layer"] = map.get_cellpoint_layer(selected_coords)
		cell_viewer["height"] = map.get_cellpoint_ground_height(selected_coords)
		cell_viewer["types"] = map.get_cellpoint_type(selected_coords)
		
		var ground_tileset = map.get_cellpoint_ground_tileset(selected_coords)
		var geo_tileset = map.get_cellpoint_geo_tileset(selected_coords)
		cell_viewer["tilesets"] = Vector2i(ground_tileset, geo_tileset)

		notify_property_list_changed()
		update_gizmos()

var brush_size: int = 0:
	set(value):
		print("brush_size ", value)
		brush_size = value
		cell_editor["brush_size"] = value
		update_gizmos()

var brush_action: BrushAction = -1:
	set(value):
		print("brush_action ",value)
		brush_action = value
		cell_editor["brush_action"] = value

func _get_property_list() -> Array[Dictionary]:
	var properties: Array[Dictionary] = [
		{
			"name": "cell_viewer",
			"type": TYPE_DICTIONARY,
			"usage": PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY
		},
		{
			"name": "cell_editor",
			"type": TYPE_DICTIONARY,
			"usage": PROPERTY_USAGE_EDITOR | PROPERTY_USAGE_READ_ONLY
		}
	]
	return properties

func brush_bbox() -> AABB:
	var src_coord = cell_viewer["coords"]
	var result_bbox: AABB = map.get_cell_bbox(src_coord)

	for y in range(brush_size):
		for x in range(brush_size):
			var coord = Vector2i(
				src_coord.x + x - brush_size / 2, 
				src_coord.y + y - brush_size / 2,
			)

			var water_height = map.get_cellpoint_water_height(coord)
			if water_height is float:
				var pos = result_bbox.position
				result_bbox = result_bbox.expand(Vector3(pos.x, water_height, pos.z))

			var bbox = map.get_cell_bbox(coord)
			if bbox is AABB:
				result_bbox = result_bbox.merge(bbox)

	return result_bbox

func do_action(): 
	print("action: %d" % [brush_action])
	match brush_action:
		BrushAction.GROUND_UP:
			change_height(selected_coords, 10)
		BrushAction.GROUND_DOWN:
			change_height(selected_coords, -10)
		BrushAction.LAYER_UP:
			change_layer(selected_coords, 1)
		BrushAction.LAYER_DOWN:
			change_layer(selected_coords, -1)
		BrushAction.RAMP_ADD:
			change_ramp(selected_coords, true)
		BrushAction.RAMP_DEL:
			change_ramp(selected_coords, false)

func change_height(coords: Vector2i, base_diff: float):
	for y in range(brush_size):
		for x in range(brush_size):
			var curr_coords = Vector2i(
				coords.x + x - brush_size / 2, 
				coords.y + y - brush_size / 2,
			)

			var div: float = 1.0 + max(
				abs(float(curr_coords.x) - coords.x), 
				abs(float(curr_coords.y) - coords.y)
			)

			var cell_height = map.get_cellpoint_ground_height(curr_coords)
			if cell_height is float:
				var new_height = cell_height + base_diff / div
				editor.set_cellpoint_ground_height(curr_coords, new_height)

func change_layer(coords: Vector2i, diff: int):
	for y in range(brush_size):
		for x in range(brush_size):
			var curr_coords = Vector2i(
				coords.x + x - brush_size / 2, 
				coords.y + y - brush_size / 2,
			)

			if diff > 0:
				editor.increase_cellpoint_layer(curr_coords)
			else:
				editor.decrease_cellpoint_layer(curr_coords)

func change_ramp(coords: Vector2i, flag: bool):
	for y in range(brush_size):
		for x in range(brush_size):
			var curr_coords = Vector2i(
				coords.x + x - brush_size / 2, 
				coords.y + y - brush_size / 2,
			)
			print("coord: %s" % [curr_coords])
			editor.set_cellpoint_ramp(curr_coords, flag)
