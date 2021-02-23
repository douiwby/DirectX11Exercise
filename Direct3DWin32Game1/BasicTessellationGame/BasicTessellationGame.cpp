#include "pch.h"
#include "BasicTessellationGame/BasicTessellationGame.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using VertexType = VertexPositionNormalUV;

void BasicTessellationGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	m_d3dContext->HSSetConstantBuffers(0, 1, m_constantBufferPerFrame.GetAddressOf());
}

void BasicTessellationGame::AddObjects()
{
	m_objects.push_back(new TessellationHill());
}

void TessellationHill::Render()
{
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(VertexType);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_4_CONTROL_POINT_PATCHLIST);

	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);

	m_d3dContext->HSSetShader(m_hullShader.Get(), nullptr, 0);
	m_d3dContext->HSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());

	m_d3dContext->DSSetShader(m_domainShader.Get(), nullptr, 0);
	m_d3dContext->DSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());

	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	m_d3dContext->PSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());
	m_d3dContext->PSSetShaderResources(0, 1, m_diffuseMapView.GetAddressOf());

	m_d3dContext->Draw(4, 0);
}

void TessellationHill::BuildShape()
{
	VertexType vertices[4] = 
	{
		{XMFLOAT3(-width / 2,0.f, depth / 2),XMFLOAT3(0.f,1.f,0.f),XMFLOAT2(0.f,0.f) },
		{XMFLOAT3( width / 2,0.f, depth / 2),XMFLOAT3(0.f,1.f,0.f),XMFLOAT2(0.f,1.f) },
		{XMFLOAT3(-width / 2,0.f,-depth / 2),XMFLOAT3(0.f,1.f,0.f),XMFLOAT2(1.f,0.f) },
		{XMFLOAT3( width / 2,0.f,-depth / 2),XMFLOAT3(0.f,1.f,0.f),XMFLOAT2(1.f,1.f) }
	};

	UINT byteWidth = 4 * sizeof(VertexType);
	CD3D11_BUFFER_DESC vbDesc(byteWidth,D3D11_BIND_VERTEX_BUFFER);

	D3D11_SUBRESOURCE_DATA vbInitData;
	vbInitData.pSysMem = vertices;
	vbInitData.SysMemPitch = 0;
	vbInitData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void TessellationHill::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_cbPerObject.material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_cbPerObject.material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
}

void TessellationHill::BuildShader()
{
	const std::wstring psFilename = L"TransparentWaveGame\\Blending.hlsl";

	m_PSByteCode = d3dUtil::CompileShader(psFilename, nullptr, "PS", "ps_5_0");
	HRESULT hr = m_d3dDevice->CreatePixelShader(m_PSByteCode->GetBufferPointer(), m_PSByteCode->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
	DX::ThrowIfFailed(hr);

	const std::wstring shaderFilename = L"BasicTessellationGame\\Tessellation.hlsl";

	m_VSByteCode = d3dUtil::CompileShader(shaderFilename, nullptr, "VS", "vs_5_0");
	hr = m_d3dDevice->CreateVertexShader(m_VSByteCode->GetBufferPointer(), m_VSByteCode->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_HSByteCode = d3dUtil::CompileShader(shaderFilename, nullptr, "HS", "hs_5_0");
	hr = m_d3dDevice->CreateHullShader(m_HSByteCode->GetBufferPointer(), m_HSByteCode->GetBufferSize(), nullptr, m_hullShader.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_DSByteCode = d3dUtil::CompileShader(shaderFilename, nullptr, "DS", "ds_5_0");
	hr = m_d3dDevice->CreateDomainShader(m_DSByteCode->GetBufferPointer(), m_DSByteCode->GetBufferSize(), nullptr, m_domainShader.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void TessellationHill::BuildTexture()
{
	BuildTextureByName(L"TransparentWaveGame\\grass.dds");
}
