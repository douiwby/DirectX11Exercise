// Include structures and functions for lighting.
#include "..\\LitHillGame\\LightHelper.hlsl"

Texture2DArray gTreeMapArray : register(t0);

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
	float3 PosW  : POSITION;
	float2 SizeW : SIZE;
};

struct VertexOut
{
	float3 CenterW : POSITION;
	float2 SizeW   : SIZE;
};

struct GeoOut
{
	float4 PosH    : SV_POSITION;
    float3 PosW    : POSITION;
    float3 NormalW : NORMAL;
    float2 TexC    : TEXCOORD;
    uint   PrimID  : SV_PrimitiveID;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;

	// Just pass data over to geometry shader.
	vout.CenterW = vin.PosW;
	vout.SizeW   = vin.SizeW;

	return vout;
}
 
 // We expand each point into a quad (4 vertices), so the maximum number of vertices
 // we output per geometry shader invocation is 4.
[maxvertexcount(4)]
void GS(point VertexOut gin[1], 
        uint primID : SV_PrimitiveID, 
        inout TriangleStream<GeoOut> triStream)
{	
	//
	// Compute the local coordinate system of the sprite relative to the world
	// space such that the billboard is aligned with the y-axis and faces the eye.
	//

	float3 up = float3(0.0f, 1.0f, 0.0f);
	float3 look = gEyePosW - gin[0].CenterW;
	look.y = 0.0f; // y-axis aligned, so project to xz-plane
	look = normalize(look);
	float3 right = cross(up, look);

	//
	// Compute triangle strip vertices (quad) in world space.
	//
	float halfWidth  = 0.5f*gin[0].SizeW.x;
	float halfHeight = 0.5f*gin[0].SizeW.y;
	
	float4 v[4];
	v[0] = float4(gin[0].CenterW + halfWidth*right - halfHeight*up, 1.0f);
	v[1] = float4(gin[0].CenterW + halfWidth*right + halfHeight*up, 1.0f);
	v[2] = float4(gin[0].CenterW - halfWidth*right - halfHeight*up, 1.0f);
	v[3] = float4(gin[0].CenterW - halfWidth*right + halfHeight*up, 1.0f);

	//
	// Transform quad vertices to world space and output 
	// them as a triangle strip.
	//
	
	float2 texC[4] = 
	{
		float2(0.0f, 1.0f),
		float2(0.0f, 0.0f),
		float2(1.0f, 1.0f),
		float2(1.0f, 0.0f)
	};
	
	GeoOut gout;
	[unroll]
	for(int i = 0; i < 4; ++i)
	{
		gout.PosH     = mul(v[i], gWorldViewProj);
		gout.PosW     = v[i].xyz;
		gout.NormalW  = look;
		gout.TexC     = texC[i];
		gout.PrimID   = primID;
		
		triStream.Append(gout);
	}
}

float4 PS(GeoOut pin) : SV_Target
{
	uint treeMapArraySize = 3;
	float3 uvw = float3(pin.TexC, pin.PrimID % treeMapArraySize);
	float4 texColor = gTreeMapArray.Sample(gSampler, uvw);

	// Debug
	//return float4(pin.TexC,1.f,0.f);
	//return texColor;

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
	float4 spec = float4(0.0f, 0.0f, 0.0f, 0.0f);

	// Sum the light contribution from each light source.
	float4 A, D, S;

	ComputeDirectionalLight(gMaterial, gDirLight, pin.NormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;

	ComputePointLight(gMaterial, gPointLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;

	ComputeSpotLight(gMaterial, gSpotLight, pin.PosW, pin.NormalW, toEyeW, A, D, S);
	ambient += A;
	diffuse += D;
	spec += S;

	float4 litColor = texColor * (ambient + diffuse) + spec;
	//litColor = texColor;

#ifdef FOG
	// Blend the fog color and the lit color.
	float fogAmount = saturate((distToEye - gFogStart) / gFogRange);
	litColor = lerp(litColor, gFogColor, fogAmount);
#endif

	// Common to take alpha from diffuse material.
	litColor.a = gMaterial.Diffuse.a;

	return litColor;
}


