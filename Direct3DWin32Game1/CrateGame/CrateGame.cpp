#include "pch.h"
#include "CrateGame/CrateGame.h"

#include "C:\\Program Files (x86)\\Microsoft DirectX SDK (June 2010)\\Include\\D3DX11tex.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void CrateGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	SetInputLayout();
	BuildCrate();
	BuildTexture();

	XMStoreFloat4x4(&m_world, XMMatrixIdentity());

	XMVECTOR pos = XMVectorSet(0.f, 2.f, -5.f, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);

	float fovAngleY = 45.f * XM_PI / 180.f;
	float aspectRatio = m_outputWidth / m_outputHeight;
	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(fovAngleY, aspectRatio, 1.f, 1000.f);
	XMStoreFloat4x4(&m_proj, perspectiveMatrix);
}

void CrateGame::SetInputLayout()
{
	ComPtr<ID3DBlob> mvsByteCode = d3dUtil::CompileShader(L"CrateGame\\Crate.hlsl", nullptr, "VS", "vs_5_0");
	ComPtr<ID3DBlob> mpsByteCode = d3dUtil::CompileShader(L"CrateGame\\Crate.hlsl", nullptr, "PS", "ps_5_0");

	m_d3dDevice->CreateVertexShader(mvsByteCode->GetBufferPointer(), mvsByteCode->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
	m_d3dDevice->CreatePixelShader(mpsByteCode->GetBufferPointer(), mpsByteCode->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}
	};

	HRESULT hr = m_d3dDevice->CreateInputLayout(
		vertexDesc,
		ARRAYSIZE(vertexDesc),
		mvsByteCode->GetBufferPointer(),
		mvsByteCode->GetBufferSize(),
		m_inputLayout.GetAddressOf()
	);
	DX::ThrowIfFailed(hr);

	m_d3dContext->IASetInputLayout(m_inputLayout.Get());

	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
}

void CrateGame::BuildCrate()
{
	// Set vertex buffer
	CrateGameVertex vertices[] =
	{
		// Front face
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(1.f,1.f) },
		// Back face
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.f,1.f) },
		// Left face
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.f,1.f) },
		// Right face
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.f,1.f) },
		// Top face
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.f,1.f) },
		// Bottom face
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.f,1.f) }
	};

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = sizeof(vertices);
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vbInitData;
	vbInitData.pSysMem = vertices;
	vbInitData.SysMemPitch = 0;
	vbInitData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	UINT stride = sizeof(CrateGameVertex);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

	// Set index buffer
	UINT indices[] =
	{
		// front face
		0, 1, 2,
		2, 1, 3,

		// back face
		4, 5, 6,
		6, 5, 7,

		// left face
		8, 9, 10,
		10, 9, 11,

		// right face
		12, 13, 14,
		14, 13, 15,

		// top face
		16, 17, 18,
		18, 17, 19,

		// bottom face
		20, 21, 22,
		22, 21, 23
	};

	m_indexCount = ARRAYSIZE(indices);

	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = sizeof(indices);
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA ibInitData;
	ibInitData.pSysMem = indices;
	ibInitData.SysMemPitch = 0;
	ibInitData.SysMemSlicePitch = 0;

	hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibInitData, m_indexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);

	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	// Set constant buffer
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(XMFLOAT4X4);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBuffer.GetAddressOf());

	m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
}

void CrateGame::BuildTexture()
{
	CD3D11_SAMPLER_DESC samplerDesc;
	ComPtr<ID3D11SamplerState> mSamplerState;
	m_d3dDevice->CreateSamplerState(&samplerDesc, mSamplerState.GetAddressOf());
	m_d3dContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

	HRESULT hr = D3DX11CreateShaderResourceViewFromFile(
		m_d3dDevice.Get(), L"CrateGame\\WoodCrate01.dds", nullptr, nullptr, m_diffuseMapView.GetAddressOf(), nullptr);
	DX::ThrowIfFailed(hr);

	m_d3dContext->PSSetShaderResources(0, 1, m_diffuseMapView.GetAddressOf());
}
