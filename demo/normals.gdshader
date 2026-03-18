shader_type spatial;

uniform sampler2D ground_texture;

void fragment()
{
	vec4 tcolor = texture(ground_texture, UV);
	ALBEDO = tcolor.xyz;
}