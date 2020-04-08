#version 430 core

out vec4 fcolor;

layout(binding = 0) uniform sampler2D base;
layout(binding = 1) uniform sampler2D metallicroughness;
layout(binding = 2) uniform sampler2D normalmap;
uniform vec3 basedcolor;
uniform vec3 campos;
uniform vec3 lightcolor = vec3(300.0, 300.0, 300.0);

in VERTEX {
	vec3 worldpos;
	vec3 normal;
	vec2 texcoord;
} fragment;

const float PI = 3.14159265359;
// ----------------------------------------------------------------------------
float DistributionGGX(vec3 N, vec3 H, float roughness)
{
    float a = roughness*roughness;
    float a2 = a*a;
    float NdotH = max(dot(N, H), 0.0);
    float NdotH2 = NdotH*NdotH;

    float nom   = a2;
    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    denom = PI * denom * denom;

    return nom / max(denom, 0.001); // prevent divide by zero for roughness=0.0 and NdotH=1.0
}
// ----------------------------------------------------------------------------
float GeometrySchlickGGX(float NdotV, float roughness)
{
    float r = (roughness + 1.0);
    float k = (r*r) / 8.0;

    float nom   = NdotV;
    float denom = NdotV * (1.0 - k) + k;

    return nom / denom;
}
// ----------------------------------------------------------------------------
float GeometrySmith(vec3 N, vec3 V, vec3 L, float roughness)
{
    float NdotV = max(dot(N, V), 0.0);
    float NdotL = max(dot(N, L), 0.0);
    float ggx2 = GeometrySchlickGGX(NdotV, roughness);
    float ggx1 = GeometrySchlickGGX(NdotL, roughness);

    return ggx1 * ggx2;
}
// ----------------------------------------------------------------------------
vec3 fresnelSchlick(float cosTheta, vec3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}


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
	vec3 basecolor = texture(base, fragment.texcoord).rgb;

	fcolor = vec4(basecolor, 1.0);
	//fcolor = vec4(basecolor, 1.0);

	float gamma = 2.2;
 	fcolor.rgb = pow(fcolor.rgb, vec3(1.0/gamma));
}
