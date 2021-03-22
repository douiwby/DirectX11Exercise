#include "pch.h"
#include "NormalMapGame/NormalMapGame.h"
#include "Common/GeometryGenerator.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using VertexType = VertexPositionNormalTangentUV;

void NormalMapGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	m_reflectObject->WorldTransform(XMMatrixTranslation(0.f, 30.f, 0.f));
}

void NormalMapGame::OnKeyButtonReleased(WPARAM key)
{
	Super::OnKeyButtonReleased(key);

	// Toggle normal map on/off
	if (key == 'N')
	{
		m_cylinder->bDisableNormalMap = !m_cylinder->bDisableNormalMap;
		m_cylinder->BuildShader();
	}
}

void NormalMapGame::AddObjects()
{
	Super::AddObjects();

	m_cylinder = new Cylinder();
	m_objects.push_back(m_cylinder);
}

void NormalMapShape::Render()
{
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(VertexType);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->VSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());

	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	m_d3dContext->PSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());
	m_d3dContext->PSSetShaderResources(0, 1, m_diffuseMapView.GetAddressOf());
	m_d3dContext->PSSetShaderResources(2, 1, m_normalMapView.GetAddressOf());

	m_d3dContext->DrawIndexed(m_indexCount, 0, 0);
}

void NormalMapShape::BuildShader()
{
	const std::wstring shaderFilename = L"NormalMapGame\\NormalMap.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename);
}

void NormalMapShape::SetInputLayout()
{
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TANGENT",0,DXGI_FORMAT_R32G32B32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXUV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0}
	};

	HRESULT hr = m_d3dDevice->CreateInputLayout(
		vertexDesc,
		ARRAYSIZE(vertexDesc),
		m_VSByteCode->GetBufferPointer(),
		m_VSByteCode->GetBufferSize(),
		m_inputLayout.GetAddressOf()
	);
	DX::ThrowIfFailed(hr);
}

void Cylinder::BuildShader()
{
	if (bDisableNormalMap)
	{
		D3D_SHADER_MACRO defines[] =
		{
			"DISABLE_NORMALMAP", "1",
			NULL, NULL
		};
		const std::wstring shaderFilename = L"NormalMapGame\\NormalMap.hlsl";
		CreateVSAndPSShader(shaderFilename, shaderFilename, defines);
	}
	else
	{
		const std::wstring shaderFilename = L"NormalMapGame\\NormalMap.hlsl";
		CreateVSAndPSShader(shaderFilename, shaderFilename);
	}
}

void Cylinder::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_cbPerObject.material.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 16.0f);
	m_cbPerObject.material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
}

void Cylinder::BuildShape()
{
	GeometryGenerator geo;
	GeometryGenerator::MeshData meshData;
	geo.CreateCylinder(5.f, 3.f, 30.0f, 20, 20, meshData);

	WorldTransform(XMMatrixTranslation(0.f, 15.f, 0.f));

	std::vector<VertexType> vertices(meshData.Vertices.size());

	for (int i = 0; i < meshData.Vertices.size(); ++i)
	{
		vertices[i].position = meshData.Vertices[i].Position;
		vertices[i].normal = meshData.Vertices[i].Normal;
		vertices[i].tangent = meshData.Vertices[i].TangentU;
		vertices[i].textureUV = meshData.Vertices[i].TexC;
	}

	std::vector<UINT> indices(meshData.Indices);

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = sizeof(VertexType) * vertices.size();
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vbInitData;
	vbInitData.pSysMem = vertices.data();
	vbInitData.SysMemPitch = 0;
	vbInitData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_indexCount = indices.size();

	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = sizeof(UINT) * indices.size();
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA ibInitData;
	ibInitData.pSysMem = indices.data();
	ibInitData.SysMemPitch = 0;
	ibInitData.SysMemSlicePitch = 0;

	hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibInitData, m_indexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void Cylinder::BuildTexture()
{
	BuildTextureByName(L"NormalMapGame\\bricks.dds", m_diffuseMapView);
	BuildTextureByName(L"NormalMapGame\\bricks_nmap.dds", m_normalMapView);
}
