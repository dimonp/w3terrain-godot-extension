shader_type spatial;
render_mode depth_draw_always, cull_back, diffuse_burley, specular_schlick_ggx;

uniform sampler2DArray texture_array: source_color;
uniform vec4 albedo : source_color;

varying float texture_index; 

void vertex() {
    texture_index = CUSTOM0.r; // Pass the data from the built-in to the varying
}

void fragment() {
    vec3 uv_with_layer = vec3(UV, texture_index);
    vec4 albedo_tex = texture(texture_array, uv_with_layer);
	ALBEDO = albedo.rgb * albedo_tex.rgb;
}
