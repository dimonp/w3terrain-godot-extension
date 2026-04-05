shader_type spatial;
render_mode blend_mix, depth_draw_always, cull_back;

uniform sampler2D noise_texture;
uniform sampler2D screen_texture : hint_screen_texture;

uniform float roughness : hint_range(0.0, 1.0) = 0.05;
uniform vec2 direction = vec2(0.1, 0.1);
uniform float strenght = 0.5;
uniform vec4 water_shallow_color = vec4(0.14, 0.12, 0.4, 0.8);
uniform vec4 water_deep_color = vec4(0.05, 0.09, 0.22, 0.96);

uniform vec4 foam_color : source_color = vec4(1.0, 1.0, 1.0, 1.0);
uniform float foam_amount = 0.1;
uniform float depth_density = 1.0;
uniform float wave_speed = 0.1;
uniform float uv_scale = 1.0;

void fragment() {
	float depth = COLOR.r;
	
	//vec3 world_pos = (MODEL_MATRIX * vec4(VERTEX, 1.0)).xyz;
    //float depth = texture(screen_texture, SCREEN_UV).r; // Assuming a depth texture is available
	
    // Apply depth based wave distortion
    vec2 distorted_uv = SCREEN_UV + (texture(noise_texture, SCREEN_UV + TIME * wave_speed).rg - 0.5) * 0.05;
    vec3 shallow_color = texture(screen_texture, distorted_uv).rgb;

    // Blend between shallow and deep water colors based on depth
    float depth_factor = exp(-depth * depth_density);
    vec3 water_color = mix(water_shallow_color.rgb, water_deep_color.rgb, depth_factor);

    // Calculate foam based on depth
    float foam_mask = 1.0 - smoothstep(0.5 - foam_amount, 0.5 + foam_amount, depth);
    vec3 final_color = mix(water_color, foam_color.rgb, foam_mask);
	
	//vec4 color = mix(water_shallow_color, water_deep_color, deep_factor);

	ALBEDO = final_color.rgb;
	ALPHA = depth;
	ROUGHNESS = roughness;

	vec2 uv_movement = direction * TIME * strenght;

	vec3 normalmap = texture(noise_texture, UV + uv_movement).rgb;
	normalmap += texture(noise_texture, UV - uv_movement).rgb;

	NORMAL_MAP = normalmap;
	NORMAL_MAP_DEPTH = 0.4;
}
