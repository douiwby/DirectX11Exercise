//***************************************************************************************
// ShadowDebug.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

Texture2D    gShadowMap : register(t0);
SamplerState gSampler : register(s0);

cbuffer cbPerObject : register(b1)
{
	float4x4 gWorldViewProj;
};

struct VertexIn
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
	float2 TexUV: TEXUV;
};

struct VertexOut
{
	float4 PosH    : SV_POSITION;
	float2 TexUV: TEXUV;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);

	vout.TexUV = vin.TexUV;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 color = gShadowMap.Sample(gSampler, pin.TexUV);
	color = float4(color.rrr, 1.f);
	return color;
}