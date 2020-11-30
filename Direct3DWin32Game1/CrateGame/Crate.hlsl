//***************************************************************************************
// color.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//
// Transforms and colors geometry.
//***************************************************************************************

Texture2D    gDiffuseMap : register(t0);
SamplerState gsamLinear  : register(s0);

cbuffer cbPerObject : register(b0)
{
	float4x4 gWorldViewProj; 
};

struct VertexIn
{
	float3 PosL  : POSITION;
    float3 Norm : NORMAL;
	float2 TextCoor: TEXCOORD;
};

struct VertexOut
{
	float4 PosH  : SV_POSITION;
	float3 Norm : NORMAL;
	float2 TextCoor: TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), gWorldViewProj);
	
    vout.Norm = vin.Norm;
	vout.TextCoor = vin.TextCoor;
    
    return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	float4 color = gDiffuseMap.Sample(gsamLinear, pin.TextCoor);
    return color;
}


