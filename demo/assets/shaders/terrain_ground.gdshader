shader_type spatial;
render_mode blend_mix, depth_draw_always, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2DArray texture_array: source_color;
uniform vec4 albedo : source_color;
uniform float alpha_scissor_threshold : hint_range(0.0, 1.0, 0.1);

varying float texture_index; 

void vertex() {
	texture_index = CUSTOM0.r;
}

void fragment() {
	vec3 uv_with_layer = vec3(UV, texture_index);
	vec4 albedo_tex = texture(texture_array, uv_with_layer);
	ALBEDO = albedo.rgb * albedo_tex.rgb;
	ALPHA = albedo.a * albedo_tex.a;
	ALPHA_SCISSOR_THRESHOLD = alpha_scissor_threshold;
}
