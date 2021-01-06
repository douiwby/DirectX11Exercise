#include "pch.h"
#include "MultiObjectGame/Shape.h"
#include "MultiObjectGame/VertexStructuer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using VertexType = VertexPositionNormalColor;

void Shape::Initialize(
	Microsoft::WRL::ComPtr<ID3D11Device>& device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
	DirectX::XMFLOAT4X4* view,
	DirectX::XMFLOAT4X4* proj)
{
	Super::Initialize(device, context, view, proj);

	BuildShader();
	SetInputLayout();
	BuildShape();
}

// Updates the model.
void Shape::Update(DX::StepTimer const& timer)
{
	XMMATRIX world = XMLoadFloat4x4(m_world);
	XMMATRIX view = XMLoadFloat4x4(m_view);
	XMMATRIX proj = XMLoadFloat4x4(m_proj);

	XMMATRIX mWorldViewProj = XMMatrixMultiply(XMMatrixMultiply(world, view), proj);
	XMFLOAT4X4 cbWorldViewProj;
	// Use XMMatrixTranspose before send to GPU due to HLSL using column-major
	XMStoreFloat4x4(&cbWorldViewProj, XMMatrixTranspose(mWorldViewProj));

	m_d3dContext->UpdateSubresource(m_constantBufferPerObject.Get(), 0, nullptr, &cbWorldViewProj, 0, 0);
}

// Draws the scene.
void Shape::Render()
{
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(VertexType);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBufferPerObject.GetAddressOf());
	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	m_d3dContext->DrawIndexed(m_indexCount, 0, 0);
}

void Shape::BuildShader()
{
	const std::wstring shaderFilename = L"HillGame\\Hill.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename);
}

void Shape::SetInputLayout()
{
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}
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

void Shape::CreateVSAndPSShader(const std::wstring & vsFilename, const std::wstring & psFilename, const D3D_SHADER_MACRO* defines/* = nullptr*/)
{
	m_VSByteCode = d3dUtil::CompileShader(vsFilename, defines, "VS", "vs_5_0");
	m_PSByteCode = d3dUtil::CompileShader(psFilename, defines, "PS", "ps_5_0");

	HRESULT hr = m_d3dDevice->CreateVertexShader(m_VSByteCode->GetBufferPointer(), m_VSByteCode->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreatePixelShader(m_PSByteCode->GetBufferPointer(), m_PSByteCode->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
	DX::ThrowIfFailed(hr);
}