shader_type spatial;
render_mode blend_mix, depth_draw_always, cull_back, shadows_disabled;

uniform float roughness : hint_range(0.0, 1.0) = 0.05;
uniform vec2 direction = vec2(0.1, 0.1);
uniform float strenght = 0.5;
uniform vec4 water_shallow_color : source_color = vec4(0.31, 0.337, 0.761, 0.875);
uniform vec4 water_deep_color : source_color = vec4(0.161, 0.239, 0.498, 0.945);

uniform sampler2D noise_texture;

void fragment() {
	float deep_factor = COLOR.r;
	vec4 color = mix(water_shallow_color, water_deep_color, deep_factor);

	ALBEDO = color.rgb;
	ALPHA = color.a;
	ROUGHNESS = roughness;

	vec2 uv_movement = direction * TIME * strenght;

	vec3 normalmap = texture(noise_texture, UV + uv_movement).rgb;
	normalmap += texture(noise_texture, UV - uv_movement).rgb;

	NORMAL_MAP = normalmap;
	NORMAL_MAP_DEPTH = 0.4;
}
