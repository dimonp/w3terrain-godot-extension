@tool
extends Container

enum CellTypes {
	GROUND	= 1 << 0,
	CLIFF	= 1 << 1,
	RAMP 	= 1 << 2,
	WATER 	= 1 << 3,
	RAMP_M 	= 1 << 4,
}

@export var cell: Dictionary:
	set(value):
		print("view cell ", value)
		
		var coord = value["coords"]
		get_node("%CoordsText").set_text("X: %d; Y: %d" % [coord.x, coord.y])
		
		var pos = value["position"]
		get_node("%PositionText").set_text("X: %.0f; Y: %.0f; Z: %.0f" % [pos.x, pos.y, pos.z])
		
		var layer = value["layer"]
		get_node("%LayerText").set_text(str(layer))
		
		var height = value["height"]
		get_node("%HeightText").set_text(str(height))
		
		var tilesets = value["tilesets"]
		get_node("%TilesetsText").set_text("Ground: %d, Geo: %d" % [tilesets.x, tilesets.y])

		var types = value["types"]
		var cell_type_ctrl = get_node("%TypesText")
		if (types & CellTypes.GROUND):
			cell_type_ctrl.text += "ground "
		if (types & CellTypes.CLIFF):
			cell_type_ctrl.text += "cliff "
		if (types & CellTypes.RAMP):
			cell_type_ctrl.text += "ramp "
		if (types & CellTypes.RAMP_M):
			cell_type_ctrl.text += "ramp "
		if (types & CellTypes.WATER):
			cell_type_ctrl.text += "water "
