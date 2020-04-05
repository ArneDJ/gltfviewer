#version 430 core

out vec4 fcolor;

layout(binding = 0) uniform sampler2D base;
layout(binding = 1) uniform sampler2D metallicroughness;
layout(binding = 2) uniform sampler2D normalmap;
uniform vec3 basedcolor;

in VERTEX {
	vec3 color;
	vec2 texcoord;
} fragment;


void main(void)
{
	vec3 basecolor = texture(base, fragment.texcoord).rgb;
	vec3 normalbump = texture(normalmap, fragment.texcoord).rgb;
	vec4 metalrough = texture(metallicroughness, fragment.texcoord);
	float occlusion = metalrough.r;
	float roughness = metalrough.g;
	float metallic = metalrough.b;
	//fcolor = vec4(vec3(metallic), 1.0);
	//fcolor = vec4(normalbump, 1.0);
	fcolor = vec4(basecolor, 1.0);
}
