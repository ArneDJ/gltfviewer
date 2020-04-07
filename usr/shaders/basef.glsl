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

/*
void main(void)
{
	vec3 viewvector = campos - fragment.worldpos;
	//vec3 normal = perturb_normal(fragment.normal, viewvector, fragment.texcoord);
	vec3 basecolor = texture(base, fragment.texcoord).rgb;
	vec3 normalbump = texture(normalmap, fragment.texcoord).rgb;
	vec4 metalrough = texture(metallicroughness, fragment.texcoord);

	// Roughness is stored in the 'g' channel, metallic is stored in the 'b' channel.
	// This layout intentionally reserves the 'r' channel for (optional) occlusion map data
	float occlusion = metalrough.r;
	float roughness = metalrough.g;
	float metallic = metalrough.b;
	fcolor = vec4(vec3(roughness), 1.0);
	//fcolor = vec4(basecolor, 1.0);
	//fcolor = vec4(basecolor, 1.0);

	float gamma = 2.2;
 	fcolor.rgb = pow(fcolor.rgb, vec3(1.0/gamma));
}
*/

void main(void)
{
	vec3 lightPositions[] = {
        vec3( 10.0f,  10.0f, 10.0f),
        vec3(-10.0f,  10.0f, 10.0f),
        vec3(-10.0f, -10.0f, 10.0f),
        vec3( 10.0f, -10.0f, 10.0f),
    };

    vec3 N = normalize(fragment.normal);
    vec3 V = normalize(campos - fragment.worldpos);
	//vec3 N = perturb_normal(fragment.normal, V, fragment.texcoord);

    // calculate reflectance at normal incidence; if dia-electric (like plastic) use F0
    // of 0.04 and if it's a metal, use the albedo color as F0 (metallic workflow)
    vec3 albedo = texture(base, fragment.texcoord).rgb;
    albedo = pow(albedo, vec3(1.2));
float metallic  = texture(metallicroughness, fragment.texcoord).b;
metallic = pow(metallic, 1.0/2.2);
float roughness = texture(metallicroughness, fragment.texcoord).g;
roughness = pow(roughness, 1.0/2.2);
//float ao = 1.0;
float ao = texture(metallicroughness, fragment.texcoord).r;
    vec3 F0 = vec3(0.04);
    F0 = mix(F0, albedo, metallic);

    // reflectance equation
    vec3 Lo = vec3(0.0);
    for(int i = 0; i < 4; ++i)
    {
        // calculate per-light radiance
        vec3 L = normalize(lightPositions[i] - fragment.worldpos);
        vec3 H = normalize(V + L);
        float distance = length(lightPositions[i] - fragment.worldpos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = lightcolor * attenuation;

        // Cook-Torrance BRDF
        float NDF = DistributionGGX(N, H, roughness);
        float G   = GeometrySmith(N, V, L, roughness);
        vec3 F    = fresnelSchlick(clamp(dot(H, V), 0.0, 1.0), F0);

        vec3 nominator    = NDF * G * F;
        float denominator = 4 * max(dot(N, V), 0.0) * max(dot(N, L), 0.0);
        vec3 specular = nominator / max(denominator, 0.001); // prevent divide by zero for NdotV=0.0 or NdotL=0.0

        // kS is equal to Fresnel
        vec3 kS = F;
        // for energy conservation, the diffuse and specular light can't
        // be above 1.0 (unless the surface emits light); to preserve this
        // relationship the diffuse component (kD) should equal 1.0 - kS.
        vec3 kD = vec3(1.0) - kS;
        // multiply kD by the inverse metalness such that only non-metals
        // have diffuse lighting, or a linear blend if partly metal (pure metals
        // have no diffuse light).
        kD *= 1.0 - metallic;

        // scale light by NdotL
        float NdotL = max(dot(N, L), 0.0);

        // add to outgoing radiance Lo
        Lo += (kD * albedo / PI + specular) * radiance * NdotL;  // note that we already multiplied the BRDF by the Fresnel (kS) so we won't multiply by kS again
    }

    // ambient lighting (note that the next IBL tutorial will replace
    // this ambient lighting with environment lighting).
    vec3 ambient = vec3(0.03) * albedo * ao;

    vec3 color = ambient + Lo;

    // HDR tonemapping
    color = color / (color + vec3(1.0));
    // gamma correct
    color = pow(color, vec3(1.0/2.2));

    fcolor = vec4(color*ao, 1.0);
    //fcolor = vec4(vec3(roughness), 1.0);
}


