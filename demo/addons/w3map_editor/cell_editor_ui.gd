@tool
extends Container

@export var button_group_brush: ButtonGroup
@export var button_group_actions: ButtonGroup

@export var cell: Dictionary:
	set(value):
		print("edit cell ", value)
		
		var brush_size = value["brush_size"]
		for btn in button_group_brush.get_buttons():
			var btn_value = btn.get_meta("brush", -1)
			if btn_value == brush_size:
				btn.set_pressed_no_signal(true)
			else:
				btn.set_pressed_no_signal(false)

		var brush_action = value["brush_action"]
		for btn in button_group_actions.get_buttons():
			var btn_value = btn.get_meta("action", -1)
			if btn_value == brush_action:
				btn.set_pressed_no_signal(true)
			else:
				btn.set_pressed_no_signal(false)
