extends Node3D

@onready var cell_coord_ctrl = $SubViewport/PanelContainerCellInfo/MarginContainer/GridContainer/LabelCoordsValue
@onready var cell_position_ctrl = $SubViewport/PanelContainerCellInfo/MarginContainer/GridContainer/LabelPosValue
@onready var cell_layer_ctrl = $SubViewport/PanelContainerCellInfo/MarginContainer/GridContainer/LabelLayerValue
@onready var cell_type_ctrl = $SubViewport/PanelContainerCellInfo/MarginContainer/GridContainer/LabelTypeValue
@onready var cell_tileset_ctrl = $SubViewport/PanelContainerCellInfo/MarginContainer/GridContainer/LabelTilesetValue

enum CellTypes {
	GROUND	= 1 << 0,
	CLIFF	= 1 << 1,
	RAMP = 1 << 2,
	WATER 	= 1 << 3
}

@export var cell_coord: Vector2i:
	set(value):
		if cell_coord_ctrl:
			cell_coord_ctrl.text = "X: %d, Y: %d" % [value.x, value.y]

@export var cell_layer: int:
	set(value):
		if cell_layer_ctrl:
			cell_layer_ctrl.text = str(value)

@export var cell_position: Vector3:
	set(value):
		if cell_position_ctrl:
			cell_position_ctrl.text = str(value)

@export var cell_tileset: Vector2i:
	set(value):
		if cell_tileset_ctrl:
			cell_tileset_ctrl.text = "ground: %d, geo: %d" % [value.x, value.y]

@export var cell_type: int:
	set(value):
		if cell_type_ctrl:
			cell_type_ctrl.text = ""
			if (value & CellTypes.GROUND):
				cell_type_ctrl.text += "ground "
			if (value & CellTypes.CLIFF):
				cell_type_ctrl.text += "cliff "
			if (value & CellTypes.RAMP):
				cell_type_ctrl.text += "ramp "
			if (value & CellTypes.WATER):
				cell_type_ctrl.text += "water "
