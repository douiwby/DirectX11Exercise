#include "pch.h"
#include "LitHillGame/LitShape.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using VertexType = VertexPositionNormalColor;

void LitShape::Initialize(
	Microsoft::WRL::ComPtr<ID3D11Device>& device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
	DirectX::XMFLOAT4X4* view,
	DirectX::XMFLOAT4X4* proj)
{
	Super::Initialize(device, context, view, proj);

	BuildMaterial();
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

	m_d3dContext->DrawIndexed(m_indexCount, 0, 0);
}

void LitShape::BuildShader()
{
	const std::wstring shaderFilename = L"LitHillGame\\Lighting.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename);
}

void LitShape::BuildMaterial()
{
	// Default material
	m_cbPerObject.material.ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(Colors::Gray);
	m_cbPerObject.material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_cbPerObject.material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
}