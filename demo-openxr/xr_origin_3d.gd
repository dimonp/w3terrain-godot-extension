extends XROrigin3D

@export var speed: float = 256.0
@export var rotation_speed: float = 1.0

@onready var camera := $XRCamera3D
@onready var left_controller := $XRController3D_Left
@onready var right_controller := $XRController3D_Right

@onready var pointer = $XRController3D_Right/FunctionPointer
@onready var raycast = $XRController3D_Right/FunctionPointer/RayCast3D

var can_trigger: bool = true

func _physics_process(delta: float):
	var trigger_val = right_controller.get_float("fire")
	if can_trigger and trigger_val > 0.5:
		can_trigger = false
		_update_selector()

	elif trigger_val == 0:
		can_trigger = true

	var input = left_controller.get_vector2(&"move")
	if input.length() > 0.1:
		rotate_y(-input.x * rotation_speed * delta)

		var forward = camera.global_transform.basis.z
		forward = forward.normalized()

		var direction = forward * -input.y
		global_position += direction * speed * delta
		
func _update_selector():
	var ray_origin = pointer.global_position
	var ray_direction = -pointer.global_transform.basis.z.normalized()
	%W3MapNode.update_selector(ray_origin, ray_direction)
