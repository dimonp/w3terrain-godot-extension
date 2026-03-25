@tool
extends EditorProperty

var ui_scene = preload("res://addons/w3map_editor/cell_editor_ui.tscn")
var ui_instance: Control

func _init():
	ui_instance = ui_scene.instantiate()
	add_child(ui_instance)
	set_bottom_editor(ui_instance) 
	
	ui_instance.get_node("GridContainerAction/ButtonGroundU").pressed.connect(_on_button_pressed_ground_up)
	ui_instance.get_node("GridContainerAction/ButtonGroundD").pressed.connect(_on_button_pressed_ground_down)
	ui_instance.get_node("GridContainerAction/ButtonLayerU").pressed.connect(_on_button_pressed_layer_up)
	ui_instance.get_node("GridContainerAction/ButtonLayerD").pressed.connect(_on_button_pressed_layer_down)
	ui_instance.get_node("HBoxContainerBrush/ButtonBrush1").toggled.connect(_on_button_toggled_ground_brush1)
	ui_instance.get_node("HBoxContainerBrush/ButtonBrush3").toggled.connect(_on_button_toggled_ground_brush3)
	ui_instance.get_node("HBoxContainerBrush/ButtonBrush5").toggled.connect(_on_button_toggled_ground_brush5)

func _on_button_pressed_ground_up():
	get_edited_object().brush_action = W3MapNodeEditor.BrushAction.GROUND_UP

func _on_button_pressed_ground_down():
	get_edited_object().brush_action = W3MapNodeEditor.BrushAction.GROUND_DOWN

func _on_button_pressed_layer_up():
	get_edited_object().brush_action = W3MapNodeEditor.BrushAction.LAYER_UP

func _on_button_pressed_layer_down():
	get_edited_object().brush_action = W3MapNodeEditor.BrushAction.LAYER_DOWN

func _on_button_unpressed_action():
	get_edited_object().brush_size = 0

func _on_button_toggled_ground_brush1(is_pressed: bool):
	get_edited_object().brush_size = 1 if is_pressed else 0
	print("_on_button_toggled_ground_brush1", is_pressed)

func _on_button_toggled_ground_brush3(is_pressed: bool):
	get_edited_object().brush_size = 3 if is_pressed else 0
	print("_on_button_toggled_ground_brush3", is_pressed)

func _on_button_toggled_ground_brush5(is_pressed: bool):
	get_edited_object().brush_size = 5 if is_pressed else 0
	print("_on_button_toggled_ground_brush5", is_pressed)

func _update_property():
	var property = get_edited_property()
	ui_instance.cell = get_edited_object()[property]
