#pragma once

struct VertexPositionNormalColor
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT4 color;
};

struct Material
{
	DirectX::XMFLOAT4 ambient;
	DirectX::XMFLOAT4 diffuse;
	DirectX::XMFLOAT4 specular; // w = SpecPower
	DirectX::XMFLOAT4 reflect;
};

struct VertexPositionNormalUV
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 textureUV;
};

struct VertexPositionNormalUVTangent
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 textureUV;
	DirectX::XMFLOAT3 tangent;
};