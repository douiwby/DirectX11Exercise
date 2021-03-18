//=============================================================================
// Lighting.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and lights geometry.
//=============================================================================

#include "..\\LitHillGame\\LightHelper.hlsl"

Texture2D gDiffuseMap : register(t0);
TextureCube gCubeMap : register(t1);
Texture2D gNormalMap : register(t2);

SamplerState gSampler : register(s0);

cbuffer cbPerFrame : register(b0)
{
	DirectionalLight gDirLight;
	PointLight gPointLight;
	SpotLight gSpotLight;
	float3 gEyePosW;

	float4 gFogColor;
	float gFogStart;
	float gFogRange;

	float2 pad;

	float4x4 gViewProj;
};

cbuffer cbPerObject : register(b1)
{
	float4x4 gWorld;
	float4x4 gWorldInvTranspose;
	float4x4 gWorldViewProj;
	Material gMaterial;
};

struct VertexIn
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
	float3 TangentL : TANGENT;
	float2 TexUV: TEXUV;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float3 TangentW : TANGENT;
	float2 TexUV: TEXUV;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to world space space.
	vout.PosW    = mul(float4(vin.PosL, 1.0f), gWorld).xyz;
	vout.NormalW = mul(vin.NormalL, (float3x3)gWorldInvTranspose);
	vout.TangentW = mul(vin.TangentL, (float3x3)gWorld);
		
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	vout.TexUV = vin.TexUV;

	return vout;
}
  
float4 PS(VertexOut pin) : SV_Target
{
	float4 texColor = gDiffuseMap.Sample(gSampler, pin.TexUV);

#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(texColor.a - 0.1f);
#endif

	// Interpolating normal can unnormalize it, so normalize it.
    pin.NormalW = normalize(pin.NormalW); 

	float3 bumpedNormalW = pin.NormalW;
#ifndef DISABLE_NORMALMAP
	// Normal mapping
	float3 normalMapSample = gNormalMap.Sample(gSampler, pin.TexUV).rgb;
	bumpedNormalW = NormalSampleToWorldSpace(normalMapSample, pin.NormalW, pin.TangentW);
#endif

	// Vector from point being lit to eye. 
	float3 toEyeW = gEyePosW - pin.PosW;
	float distToEye = length(toEyeW);
	toEyeW /= distToEye; // normalize

	// Start with a sum of zero. 
	float4 ambient = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 diffuse = float4(0.0f, 0.0f, 0.0f, 0.0f);
	float4 spec    = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Sum the light contribution from each light source.
	float4 A, D, S;

	ComputeDirectionalLight(gMaterial, gDirLight, bumpedNormalW, toEyeW, A, D, S);
	ambient += A;  
	diffuse += D;
	spec    += S;

	ComputePointLight(gMaterial, gPointLight, pin.PosW, bumpedNormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec    += S;

	ComputeSpotLight(gMaterial, gSpotLight, pin.PosW, bumpedNormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec    += S;
	
	float4 litColor = texColor * (ambient + diffuse) + spec;
	//litColor = texColor;

#ifdef SKY_REFLECTION
	float3 incident = -toEyeW;
	float3 reflectionVector = reflect(incident, pin.NormalW);
	float4 reflectionColor = gCubeMap.Sample(gSampler, reflectionVector);

	litColor += gMaterial.Reflect*reflectionColor;
#endif

#ifdef FOG
	// Blend the fog color and the lit color.
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

	// Common to take alpha from diffuse material.
	litColor.a = gMaterial.Diffuse.a;

    return litColor;
}