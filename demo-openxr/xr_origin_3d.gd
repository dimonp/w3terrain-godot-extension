extends XROrigin3D

@export var speed: float = 128.0

@onready var camera := $XRCamera3D
@onready var right_controller := $XRController3D_Right

func _physics_process(delta: float):
	var input = right_controller.get_vector2(&"move")
	if input.length() > 0.1:
		var direction = (camera.global_transform.basis.z * -input.y + 
					 camera.global_transform.basis.x * input.x)

		global_position += direction * speed * delta
