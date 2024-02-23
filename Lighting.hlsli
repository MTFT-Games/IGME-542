// Include guard because pragma once doesnt work in hlsl
#ifndef __LIGHTING_HELPERS_INCLUDE__
#define __LIGHTING_HELPERS_INCLUDE__

#define LIGHT_TYPE_DIRECTIONAL 0
#define LIGHT_TYPE_POINT       1
#define LIGHT_TYPE_SPOT        2
#define MAX_SPECULAR_EXPONENT 256.0f

// A constant Fresnel value for non-metals (glass and plastic have values of about 0.04)
static const float F0_NON_METAL = 0.04f;

// Minimum roughness for when spec distribution function denominator goes to zero
static const float MIN_ROUGHNESS = 0.0000001f; // 6 zeros after decimal

// Handy to have this as a constant
static const float PI = 3.14159265359f;

struct Light
{
	int Type;
	float3 Direction;
	float Range;
	float3 Position;
	float intensity;
	float3 Color;
	float SpotFalloff;
	float3 Padding; // For 16-byte boundary
};

// Calculates direction to directional light
float3 ToDirectionalLight(Light light)
{
	return normalize(-light.Direction);
}

// Calculates diffuse
float Diffuse(float3 normal, float3 dirToLight)
{
	float diffuse = dot(normal, dirToLight);
	return saturate(diffuse);
}

// Calculates the ambient color of lighting with the ambient term and surface color
// DEPRICATED: Not to be used with PBR. Only for the case that simple lighting is re-added later
float3 AmbientColor(float3 ambient, float3 surfaceColor)
{
	return ambient * surfaceColor;
}

// Calculates diffuse color for a directional light in one step 
// DEPRICATED: Not to be used with PBR.
float3 DirectionalDiffuseColor(float3 normal, Light light, float3 surfaceColor)
{
	return Diffuse(normal, ToDirectionalLight(light)) * light.Color * surfaceColor;
}

// Calculates vactor from pixel to camera I think?
float3 ViewVector(float3 cameraPos, float3 worldPos)
{
	return normalize(cameraPos - worldPos);
}

// Calculates diffuse color for a point light in one step
// DEPRICATED: Not to be used with PBR.
float3 PointDiffuseColor(float3 normal, Light light, float3 surfaceColor, float3 worldPos)
{
	float3 toPointLight = normalize(light.Position - worldPos);
	return Diffuse(normal, toPointLight) * light.Color * surfaceColor;
}

// Calculates specular color for a directional light
// DEPRICATED: Not to be used with PBR.
float3 DirectionalSpecular(float3 normal, float3 cameraPos, Light light, float roughness, float3 view)
{
	float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
	if (specExponent < 0.05) 
	{
		return 0;
	}
	float3 R = reflect(light.Direction, normal);
	return pow(saturate(dot(R, view)), specExponent);
}

// Calculates specular color for a point light
// DEPRICATED: Not to be used with PBR.
float3 PointSpecular(float3 normal, float3 cameraPos, Light light, float roughness, float3 view, float3 worldPos)
{
	float specExponent = (1.0f - roughness) * MAX_SPECULAR_EXPONENT;
	if (specExponent < 0.05)
	{
		return 0;
	}
	float3 fromPointLight = normalize(worldPos - light.Position);
	float3 R = reflect(fromPointLight, normal);
	return pow(saturate(dot(R, view)), specExponent);
}

// Calculate attenuation value for light from a point light.
float Attenuate(Light light, float3 worldPos)
{
	float dist = distance(light.Position, worldPos);
	float att = saturate(1.0f - (dist * dist / (light.Range * light.Range)));
	return att * att;
}

#define GAMMA 2.2f

// Corrects a color for gamma before displaying
// Broken into its own finction for readability and 
// so it can be changed later in one spot. (also keeps me from having to 
// remember which side raises to inverse)
float3 GammaCorrect(float3 color)
{
	//TODO: make this adjustable, maybe by player
	return pow(color, 1.0f / GAMMA);
}

// Undoes gamma correction on a color, primarily for textures so lighting calculations can run with linear colors
// Broken into its own finction for readability and 
// so it can be changed later in one spot
float3 GammaUncorrect(float3 color)
{
	//TODO: make this adjustable, maybe by player
	return pow(color, GAMMA);
}

// PBR FUNCTIONS ================


// Calculates diffuse amount based on energy conservation
//
// diffuse   - Diffuse amount
// F         - Fresnel result from microfacet BRDF
// metalness - surface metalness amount 
float3 DiffuseEnergyConserve(float3 diffuse, float3 F, float metalness)
{
	return diffuse * (1 - F) * (1 - metalness);
}



// Normal Distribution Function: GGX (Trowbridge-Reitz)
//
// a - Roughness
// h - Half vector
// n - Normal
// 
// D(h, n, a) = a^2 / pi * ((n dot h)^2 * (a^2 - 1) + 1)^2
float D_GGX(float3 n, float3 h, float roughness)
{
	// Pre-calculations
	float NdotH = saturate(dot(n, h));
	float NdotH2 = NdotH * NdotH;
	float a = roughness * roughness;
	float a2 = max(a * a, MIN_ROUGHNESS); // Applied after remap!

	// ((n dot h)^2 * (a^2 - 1) + 1)
	// Can go to zero if roughness is 0 and NdotH is 1
	float denomToSquare = NdotH2 * (a2 - 1) + 1;

	// Final value
	return a2 / (PI * denomToSquare * denomToSquare);
}



// Fresnel term - Schlick approx.
// 
// v - View vector
// h - Half vector
// f0 - Value when l = n
//
// F(v,h,f0) = f0 + (1-f0)(1 - (v dot h))^5
float3 F_Schlick(float3 v, float3 h, float3 f0)
{
	// Pre-calculations
	float VdotH = saturate(dot(v, h));

	// Final value
	return f0 + (1 - f0) * pow(1 - VdotH, 5);
}



// Geometric Shadowing - Schlick-GGX
// - k is remapped to a / 2, roughness remapped to (r+1)/2 before squaring!
//
// n - Normal
// v - View vector
//
// G_Schlick(n,v,a) = (n dot v) / ((n dot v) * (1 - k) * k)
//
// Full G(n,v,l,a) term = G_SchlickGGX(n,v,a) * G_SchlickGGX(n,l,a)
float G_SchlickGGX(float3 n, float3 v, float roughness)
{
	// End result of remapping:
	float k = pow(roughness + 1, 2) / 8.0f;
	float NdotV = saturate(dot(n, v));

	// Final value
	// Note: Numerator should be NdotV (or NdotL depending on parameters).
	// However, these are also in the BRDF's denominator, so they'll cancel!
	// We're leaving them out here AND in the BRDF function as the
	// dot products can get VERY small and cause rounding errors.
	return 1 / (NdotV * (1 - k) + k);
}



// Cook-Torrance Microfacet BRDF (Specular)
//
// f(l,v) = D(h)F(v,h)G(l,v,h) / 4(n dot l)(n dot v)
// - parts of the denominator are canceled out by numerator (see below)
//
// D() - Normal Distribution Function - Trowbridge-Reitz (GGX)
// F() - Fresnel - Schlick approx
// G() - Geometric Shadowing - Schlick-GGX
float3 MicrofacetBRDF(float3 n, float3 l, float3 v, float roughness, float3 f0, out float3 F_out)
{
	// Other vectors
	float3 h = normalize(v + l);

	// Run numerator functions
	float  D = D_GGX(n, h, roughness);
	float3 F = F_Schlick(v, h, f0);
	float  G = G_SchlickGGX(n, v, roughness) * G_SchlickGGX(n, l, roughness);

	// Pass F out of the function for diffuse balance
	F_out = F;

	// Final specular formula
	// Note: The denominator SHOULD contain (NdotV)(NdotL), but they'd be
	// canceled out by our G() term.  As such, they have been removed
	// from BOTH places to prevent floating point rounding errors.
	float3 specularResult = (D * F * G) / 4;

	// One last non-obvious requirement: According to the rendering equation,
	// specular must have the same NdotL applied as diffuse!  We'll apply
	// that here so that minimal changes are required elsewhere.
	return specularResult * max(dot(n, l), 0);
}

//These feel so much better than whatever I did for the old lighting
float3 HandleDirectionalLightPBR(float3 normal, Light light, float3 toCamera, float roughness, float metalness, float3 specColor, float3 diffColor)
{
    float3 toDirLight = ToDirectionalLight(light);
	
    float3 fresnelResult; // Needed later for energy conservation
    float3 specular = MicrofacetBRDF(normal, toDirLight, toCamera, roughness, specColor, fresnelResult);
    float3 diffuse = Diffuse(normal, toDirLight);
    diffuse = DiffuseEnergyConserve(diffuse, fresnelResult, metalness) * diffColor;
	
	
	// No ambient so just add spec and diff
    return (specular + diffuse) * light.intensity * light.Color;
}

float3 HandlePointLightPBR(float3 normal, Light light, float3 toCamera, float roughness, float metalness, float3 specColor, float3 diffColor, float3 worldPos)
{
    float3 toPointLight = normalize(light.Position - worldPos);
	
    float3 fresnelResult; // Needed later for energy conservation
    float3 specular = MicrofacetBRDF(normal, toPointLight, toCamera, roughness, specColor, fresnelResult);
    float3 diffuse = Diffuse(normal, toPointLight);
    diffuse = DiffuseEnergyConserve(diffuse, fresnelResult, metalness) * diffColor;
	
	
	// No ambient so just add spec and diff
    return (specular + diffuse) * light.intensity * light.Color * Attenuate(light, worldPos);
}
#endif