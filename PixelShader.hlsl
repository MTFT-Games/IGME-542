#include "Lighting.hlsli"
// Struct representing the data we expect to receive from earlier pipeline stages
// - Should match the output of our corresponding vertex shader
// - The name of the struct itself is unimportant
// - The variable names don't have to match other shaders (just the semantics)
// - Each variable must have a semantic, which defines its usage
struct VertexToPixel
{
	// Data type
	//  |
	//  |   Name          Semantic
	//  |    |                |
	//  v    v                v
    float4 screenPosition : SV_POSITION; // XYZW position (System Value Position)
    float2 uv             : TEXCOORD;
    float3 normal         : NORMAL;
    float3 tangent        : TANGENT;
    float3 worldPos       : POSITION;
};

cbuffer ExternalData : register(b0)
{
    float2 uvScale;
    float2 uvOffset;
    float3 cameraPosition;
    int lightCount;
    Light lights[20];
}

Texture2D Albedo : register(t0);
Texture2D NormalMap : register(t1);
Texture2D MetalnessMap : register(t2);
Texture2D RoughnessMap : register(t3);

SamplerState Sampler : register(s0);

// --------------------------------------------------------
// The entry point (main method) for our pixel shader
// 
// - Input is the data coming down the pipeline (defined by the struct)
// - Output is a single color (float4)
// - Has a special semantic (SV_TARGET), which means 
//    "put the output of this into the current render target"
// - Named "main" because that's the default the shader compiler looks for
// --------------------------------------------------------
float4 main(VertexToPixel input) : SV_TARGET
{
    // Rasterizer doesn't give normalized vectors
    input.normal = normalize(input.normal);
    input.tangent = normalize(input.tangent);
    
    // Normal map
	// Unpack from 0-1 color values into -1 to 1 vectors
    float3 unpackedNormal = NormalMap.Sample(Sampler, input.uv).rgb * 2 - 1;
    unpackedNormal = normalize(unpackedNormal); // Don't forget to normalize!
    
    input.tangent = normalize(input.tangent - input.normal * dot(input.tangent, input.normal)); // Gram-Schmidt assumes T&N are normalized! This orthonormalizes
    float3 bitangent = cross(input.tangent, input.normal);
    float3x3 TBN = float3x3(input.tangent, bitangent, input.normal);
    input.normal = mul(unpackedNormal, TBN); // Note multiplication order!
    
    float3 surfaceColor = GammaUncorrect(Albedo.Sample(Sampler, input.uv).rgb);// * colorTint.rgb;
    float3 view = ViewVector(cameraPosition, input.worldPos);
    
    float roughness = RoughnessMap.Sample(Sampler, input.uv).r;
    float metalness = MetalnessMap.Sample(Sampler, input.uv).r;
    
    // Specular color determination -----------------
	// Assume albedo texture is actually holding specular color where metalness == 1
	// Note the use of lerp here - metal is generally 0 or 1, but might be in between
	// because of linear texture sampling, so we lerp the specular color to match
    float3 specularColor = lerp(F0_NON_METAL, surfaceColor.rgb, metalness);
    
    float3 litPixel;
    
    for (int i = 0; i < lightCount; i++)
    {
        if (lights[i].Type == 0)
        {
            litPixel += HandleDirectionalLightPBR(input.normal, lights[i], view, roughness, metalness, specularColor, surfaceColor);
        }
        else
        {
            litPixel += HandlePointLightPBR(input.normal, lights[i], view, roughness, metalness, specularColor, surfaceColor, input.worldPos);
        }
    }
    return float4(GammaCorrect(litPixel), 1);
}