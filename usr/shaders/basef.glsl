#version 430 core

out vec4 fcolor;

layout(binding = 0) uniform sampler2D base;
layout(binding = 1) uniform sampler2D metallicroughness;
layout(binding = 2) uniform sampler2D normalmap;
uniform vec3 basedcolor;
uniform vec3 campos;

in VERTEX {
	vec3 worldpos;
	vec3 normal;
	vec2 texcoord;
} fragment;

mat3 cotangent_frame( vec3 N, vec3 p, vec2 uv ) 
{ 
	// get edge vectors of the pixel triangle 
	vec3 dp1 = dFdx( p ); 
	vec3 dp2 = dFdy( p ); 
	vec2 duv1 = dFdx( uv ); 
	vec2 duv2 = dFdy( uv );   
	// solve the linear system 
	vec3 dp2perp = cross( dp2, N ); 
	vec3 dp1perp = cross( N, dp1 ); 
	vec3 T = dp2perp * duv1.x + dp1perp * duv2.x; 
	vec3 B = dp2perp * duv1.y + dp1perp * duv2.y;   
	// construct a scale-invariant frame 
	float invmax = inversesqrt( max( dot(T,T), dot(B,B) ) ); 
	return mat3( T * invmax, B * invmax, N ); 
}

vec3 perturb_normal( vec3 N, vec3 V, vec2 texcoord ) 
{ 
	// assume N, the interpolated vertex normal and 
	// // V, the view vector (vertex to eye) 
	vec3 map = texture(normalmap, fragment.texcoord).rgb; 
#ifdef WITH_NORMALMAP_UNSIGNED 
	map = map * 255./127. - 128./127.; 
#endif 
#ifdef WITH_NORMALMAP_2CHANNEL 
	map.z = sqrt( 1. - dot( map.xy, map.xy ) ); 
#endif 
#ifdef WITH_NORMALMAP_GREEN_UP 
	map.y = -map.y; 
#endif 
	mat3 TBN = cotangent_frame(N, -V, fragment.texcoord); 
	return normalize( TBN * map ); 
}

void main(void)
{
	vec3 viewvector = campos - fragment.worldpos;
	vec3 normal = perturb_normal(fragment.normal, viewvector, fragment.texcoord);
	vec3 basecolor = texture(base, fragment.texcoord).rgb;
	//vec3 normalbump = texture(normalmap, fragment.texcoord).rgb;
	vec4 metalrough = texture(metallicroughness, fragment.texcoord);
	float occlusion = metalrough.r;
	float roughness = metalrough.g;
	float metallic = metalrough.b;
	//fcolor = vec4(vec3(metallic), 1.0);
	//fcolor = vec4(normalbump, 1.0);
	fcolor = vec4(normal, 1.0);

	float gamma = 1.6;
 	fcolor.rgb = pow(fcolor.rgb, vec3(1.0/gamma));
}
