#include "pch.h"
#include "LitHillGame/LitShape.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

// Example
#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
#endif

#if USE_VERTEX_COLOR

using VertexType = VertexPositionNormalColor;

#elif USE_TEXTURE_UV

using VertexType = VertexPositionNormalUV;

#define USE_DirectXTK 1

#if USE_DirectXTK
// Tutorial on how to add DirectXTK
// https://github.com/microsoft/DirectXTK/wiki/Adding-the-DirectX-Tool-Kit
#include "DDSTextureLoader.h"
#else 
#error "Define how to load texture!"
#endif

#else 
#error "Define VertexType!"
#endif



void LitShape::Initialize(
	Microsoft::WRL::ComPtr<ID3D11Device>& device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
	DirectX::XMFLOAT4X4* view,
	DirectX::XMFLOAT4X4* proj)
{
	Super::Initialize(device, context, view, proj);

	BuildMaterial();

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
	BuildTexture();
#endif
}

void LitShape::Update(DX::StepTimer const & timer)
{
	XMMATRIX world = XMLoadFloat4x4(m_world);
	XMMATRIX view = XMLoadFloat4x4(m_view);
	XMMATRIX proj = XMLoadFloat4x4(m_proj);

	world.r[3] = XMVectorSet(0.0f, 0.0f, 0.0f, 1.0f);
	XMVECTOR det = XMMatrixDeterminant(world);
	XMMATRIX worldInvTranspose = XMMatrixTranspose(XMMatrixInverse(&det, world));
	XMMATRIX worldViewProj = XMMatrixMultiply(XMMatrixMultiply(world, view), proj);

	// Use XMMatrixTranspose before send to GPU due to HLSL using column-major
	XMStoreFloat4x4(&m_cbPerObject.world, XMMatrixTranspose(world));
	XMStoreFloat4x4(&m_cbPerObject.worldInvTranspose, XMMatrixTranspose(worldInvTranspose));
	XMStoreFloat4x4(&m_cbPerObject.worldViewProj, XMMatrixTranspose(worldViewProj));

	m_d3dContext->UpdateSubresource(m_constantBufferPerObject.Get(), 0, nullptr, &m_cbPerObject, 0, 0);
}

void LitShape::Render()
{
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(VertexType);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_d3dContext->VSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());
	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->PSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());
	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
	m_d3dContext->PSSetShaderResources(0, 1, m_diffuseMapView.GetAddressOf());
#endif

	m_d3dContext->DrawIndexed(m_indexCount, 0, 0);
}

void LitShape::BuildShader()
{
	const std::wstring shaderFilename = L"LitHillGame\\Lighting.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename);
}

void LitShape::SetInputLayout()
{
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
#ifdef USE_VERTEX_COLOR
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}
#else
		{"TEXUV", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}
#endif
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

void LitShape::BuildMaterial()
{
	// Default material
	m_cbPerObject.material.ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(Colors::Gray);
	m_cbPerObject.material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_cbPerObject.material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
}
#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
void LitShape::BuildTextureByName(const wchar_t * fileName)
{
#if USE_DirectXTK
	HRESULT hr = CreateDDSTextureFromFile(m_d3dDevice.Get(), fileName, nullptr, m_diffuseMapView.GetAddressOf());
#elif USE_D3DX
	HRESULT hr = D3DX11CreateShaderResourceViewFromFile(
		m_d3dDevice.Get(), fileName, nullptr, nullptr, m_diffuseMapView.GetAddressOf(), nullptr);
#endif

	DX::ThrowIfFailed(hr);
}
#endif
