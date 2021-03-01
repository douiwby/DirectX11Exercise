//=============================================================================
// Lighting.fx by Frank Luna (C) 2011 All Rights Reserved.
//
// Transforms and lights geometry.
//=============================================================================

#include "..\\LitHillGame\\LightHelper.hlsl"

struct InstanceData
{
	float4x4 World;
	uint     MaterialIndex;
	uint     InstPad0;
	uint     InstPad1;
	uint     InstPad2;
};

struct MaterialData
{
	Material Mat;
	uint     DiffuseMapIndex;
	uint     MatPad0;
	uint     MatPad1;
	uint     MatPad2;
};

Texture2DArray gDiffuseMapArray : register(t0);

StructuredBuffer<InstanceData> gInstanceData : register(t1);
StructuredBuffer<MaterialData> gMaterialData : register(t2);

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

//cbuffer cbPerObject : register(b1)
//{
//	float4x4 gWorld;
//	float4x4 gWorldInvTranspose;
//	float4x4 gWorldViewProj;
//	Material gMaterial;
//};

struct VertexIn
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
	float2 TexUV: TEXUV;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
	float2 TexUV: TEXUV;

	// nointerpolation is used so the index is not interpolated 
	// across the triangle.
	nointerpolation uint MatIndex  : MATINDEX;
};

VertexOut VS(VertexIn vin, uint instanceID : SV_InstanceID)
{
	VertexOut vout;
	
	// Fetch the instance data.
	InstanceData instData = gInstanceData[instanceID];
	float4x4 world = instData.World;
	uint matIndex = instData.MaterialIndex;

	vout.MatIndex = matIndex;

	// Transform to world space space.
	float4 posW = mul(float4(vin.PosL, 1.0f), world);
	vout.PosW    = posW.xyz;
	vout.NormalW = mul(vin.NormalL, (float3x3)world);
		
	// Transform to homogeneous clip space.
	vout.PosH = mul(posW, gViewProj);

	vout.TexUV = vin.TexUV;

	return vout;
}
  
float4 PS(VertexOut pin) : SV_Target
{
	MaterialData matData = gMaterialData[pin.MatIndex];

	float4 texColor = gDiffuseMapArray.Sample(gSampler, float3(pin.TexUV, matData.DiffuseMapIndex));
	Material mat = matData.Mat;

#ifdef ALPHA_TEST
	// Discard pixel if texture alpha < 0.1.  We do this test as soon 
	// as possible in the shader so that we can potentially exit the
	// shader early, thereby skipping the rest of the shader code.
	clip(texColor.a - 0.1f);
#endif

	// Interpolating normal can unnormalize it, so normalize it.
    pin.NormalW = normalize(pin.NormalW); 

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

	ComputeDirectionalLight(mat, gDirLight, pin.NormalW, toEyeW, A, D, S);
	ambient += A;  
	diffuse += D;
	spec    += S;

	ComputePointLight(mat, gPointLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec    += S;

	ComputeSpotLight(mat, gSpotLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec    += S;
	
	float4 litColor = texColor * (ambient + diffuse) + spec;
	//litColor = texColor;

#ifdef FOG
	// Blend the fog color and the lit color.
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

	// Common to take alpha from diffuse material.
	litColor.a = mat.Diffuse.a;

    return litColor;
}