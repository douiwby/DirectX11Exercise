#include "pch.h"
#include "Common/Shape.h"
#include "Common/VertexStructuer.h"

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
	BuildConstantBuffer();
}

// Updates the model.
void Shape::Update(DX::StepTimer const& timer)
{
	UpdateConstantBufferPerObject();
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

void Shape::CreateConstantBufferPerObject(UINT bufferSize)
{
	// Set constant buffer
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = bufferSize;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBufferPerObject.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void Shape::UpdateConstantBufferPerObject()
{
	XMMATRIX world = XMLoadFloat4x4(m_world);
	XMMATRIX view = XMLoadFloat4x4(m_view);
	XMMATRIX proj = XMLoadFloat4x4(m_proj);

	XMMATRIX mWorldViewProj = XMMatrixMultiply(XMMatrixMultiply(world, view), proj);
	XMFLOAT4X4 cbWorldViewProj;
	// Use XMMatrixTranspose before send to GPU due to HLSL using column-major
	XMStoreFloat4x4(&cbWorldViewProj, XMMatrixTranspose(mWorldViewProj));

	d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerObject, cbWorldViewProj);
}

void Shape::SetVSAndPSShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>& VS, Microsoft::WRL::ComPtr<ID3D11PixelShader>& PS)
{
	HRESULT hr;
	if (VS)
	{
		m_vertexShader = VS;
	}
	if (PS)
	{
		m_pixelShader = PS;
	}
}
