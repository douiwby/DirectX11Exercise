//***************************************************************************************
// Tessellation.hlsl by Frank Luna (C) 2015 All Rights Reserved.
//***************************************************************************************

#include "LitHillGame/LightHelper.hlsl"

cbuffer cbPerFrame : register(b0)
{
	DirectionalLight gDirLight;
	PointLight gPointLight;
	SpotLight gSpotLight;
	float3 gEyePosW;
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
	float2 TexUV: TEXUV;
};

struct VertexOut
{
	float3 PosL  : POSITION;
	float3 NormalL : NORMAL;
	float2 TexUV: TEXUV;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	vout.PosL = vin.PosL;
	vout.NormalL = vin.NormalL;
	vout.TexUV = vin.TexUV;

	return vout;
}

struct PatchTess
{
	float EdgeTess[4]   : SV_TessFactor;
	float InsideTess[2] : SV_InsideTessFactor;
};

PatchTess ConstantHS(InputPatch<VertexOut, 4> patch, uint patchID : SV_PrimitiveID)
{
	PatchTess pt;

	float3 centerL = 0.25f*(patch[0].PosL + patch[1].PosL + patch[2].PosL + patch[3].PosL);
	float3 centerW = mul(float4(centerL, 1.0f), gWorld).xyz;

	float d = distance(centerW, gEyePosW);

	// Tessellate the patch based on distance from the eye such that
	// the tessellation is 0 if d >= d1 and 64 if d <= d0.  The interval
	// [d0, d1] defines the range we tessellate in.

	const float d0 = 20.0f;
	const float d1 = 500.0f;
	float tess = 64.0f*saturate((d1 - d) / (d1 - d0));

	// Uniformly tessellate the patch.

	pt.EdgeTess[0] = tess;
	pt.EdgeTess[1] = tess;
	pt.EdgeTess[2] = tess;
	pt.EdgeTess[3] = tess;

	pt.InsideTess[0] = tess;
	pt.InsideTess[1] = tess;

	return pt;
}

struct HullOut
{
	float3 PosL : POSITION;
};

[domain("quad")]
//[partitioning("integer")]
[partitioning("fractional_even")]
//[partitioning("fractional_odd")]
[outputtopology("triangle_cw")]
[outputcontrolpoints(4)]
[patchconstantfunc("ConstantHS")]
[maxtessfactor(64.0f)]
HullOut HS(InputPatch<VertexOut, 4> p,
	uint i : SV_OutputControlPointID,
	uint patchId : SV_PrimitiveID)
{
	HullOut hout;

	hout.PosL = p[i].PosL;

	return hout;
}

struct DomainOut
{
	float4 PosH    : SV_POSITION;
	float3 PosW    : POSITION;
	float3 NormalW : NORMAL;
	float2 TexUV   : TEXUV;
};

// The domain shader is called for every vertex created by the tessellator.  
// It is like the vertex shader after tessellation.
[domain("quad")]
DomainOut DS(PatchTess patchTess,
	float2 uv : SV_DomainLocation,
	const OutputPatch<HullOut, 4> quad)
{
	DomainOut dout;

	// Bilinear interpolation.
	float3 v1 = lerp(quad[0].PosL, quad[1].PosL, uv.x);
	float3 v2 = lerp(quad[2].PosL, quad[3].PosL, uv.x);
	float3 p = lerp(v1, v2, uv.y);

	// Displacement mapping
	p.y = 0.3f*(p.z*sin(0.1f*p.x) + p.x*cos(0.1f*p.z));

	dout.PosW = mul(float4(p, 1.0f), gWorld);
	dout.PosH = mul(float4(p, 1.0f), gWorldViewProj);

	// Calculate normal
	float3 NormalL = float3(
		-0.03f*p.z*cos(0.1f*p.x) - 0.3f*cos(0.1f*p.z),
		1.0f,
		-0.3f*sin(0.1f*p.x) + 0.03f*p.x*sin(0.1f*p.z));
	NormalL = normalize(NormalL);
	dout.NormalW = mul(NormalL, (float3x3)gWorldInvTranspose);

	dout.TexUV = uv;

	return dout;
}