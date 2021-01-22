#include "pch.h"
#include "TransparentWaveGame/TransparentWaveGame.h"

#define ENABLEFOG 0

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using VertexType = VertexPositionNormalUV;

void TransparentWaveGame::Initialize(HWND window, int width, int height)
{

	m_initCameraY = 150.f;
	m_initCameraZ = -150.f;
	m_maxRadius = 300.f;
	m_mouseMoveRate = 15.f;

	Super::Initialize(window, width, height);

	// Set per frame constant buffer
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(cbPerFrame);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBufferPerFrame.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_d3dContext->PSSetConstantBuffers(0, 1, m_constantBufferPerFrame.GetAddressOf());
}

void TransparentWaveGame::BuildLight()
{
	Super::BuildLight();

	m_dirLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pointLight.Diffuse = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	m_spotLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
}

void TransparentWaveGame::Update(DX::StepTimer const & timer)
{
	Super::Update(timer);

	m_cbPerFrame.dirLight = m_dirLight;
	m_cbPerFrame.pointLight = m_pointLight;
	m_cbPerFrame.spotLight = m_spotLight;
	m_cbPerFrame.eyePosW = m_eyePos;
	m_cbPerFrame.fogColor = XMFLOAT4(Colors::Silver);
	m_cbPerFrame.fogStart = 15.f;
	m_cbPerFrame.fogRange = 175.f;
	m_d3dContext->UpdateSubresource(m_constantBufferPerFrame.Get(), 0, nullptr, &m_cbPerFrame, 0, 0);
}

void TransparentWaveGame::AddObjects()
{
	m_objects.push_back(new TextureHill());
	m_objects.push_back(new Crate());
	m_objects.push_back(new TransparentWave());
}

void TextureHill::BuildShader()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
#if ENABLEFOG
		"FOG", "1",
#endif
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const std::wstring shaderFilename = L"TransparentWaveGame\\Blending.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename, alphaTestDefines);
}

void TransparentWave::BuildShader()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
#if ENABLEFOG
		"FOG", "1",
#endif
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const std::wstring shaderFilename = L"TransparentWaveGame\\Blending.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename, alphaTestDefines);
}

void TransparentWave::Render()
{
	CD3D11_BLEND_DESC bd(D3D11_DEFAULT);
	bd.RenderTarget[0] =
	{
		TRUE,
		D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};
	ComPtr<ID3D11BlendState> mBlendState;
	m_d3dDevice->CreateBlendState(&bd, mBlendState.GetAddressOf());
	float blendFactors[] = { 0.f,0.f,0.f,0.f };
	m_d3dContext->OMSetBlendState(mBlendState.Get(), blendFactors, UINT_MAX);

	ComPtr<ID3D11SamplerState> previousSamplerState;
	m_d3dContext->PSGetSamplers(0, 1, previousSamplerState.GetAddressOf());
	CD3D11_SAMPLER_DESC samplerDesc(D3D11_DEFAULT);
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	ComPtr<ID3D11SamplerState> mSamplerState;
	HRESULT hr = m_d3dDevice->CreateSamplerState(&samplerDesc, mSamplerState.GetAddressOf());
	DX::ThrowIfFailed(hr);
	m_d3dContext->PSSetSamplers(0, 1, mSamplerState.GetAddressOf());

	Super::Render();
	
	m_d3dContext->OMSetBlendState(nullptr, blendFactors, UINT_MAX);
	m_d3dContext->PSSetSamplers(0, 1, previousSamplerState.GetAddressOf());
}

void Crate::Render()
{
	CD3D11_BLEND_DESC bd(D3D11_DEFAULT);
	bd.RenderTarget[0] =
	{
		TRUE,
		D3D11_BLEND_SRC_ALPHA, D3D11_BLEND_INV_SRC_ALPHA, D3D11_BLEND_OP_ADD,
		D3D11_BLEND_ONE, D3D11_BLEND_ZERO, D3D11_BLEND_OP_ADD,
		D3D11_COLOR_WRITE_ENABLE_ALL
	};
	ComPtr<ID3D11BlendState> mBlendState;
	m_d3dDevice->CreateBlendState(&bd, mBlendState.GetAddressOf());
	float blendFactors[] = { 0.f,0.f,0.f,0.f };
	m_d3dContext->OMSetBlendState(mBlendState.Get(), blendFactors, UINT_MAX);


	// Disable back face culling
	ComPtr<ID3D11RasterizerState> previousRSState;
	m_d3dContext->RSGetState(previousRSState.GetAddressOf());
	CD3D11_RASTERIZER_DESC rsDesc(D3D11_DEFAULT);
	rsDesc.CullMode = D3D11_CULL_NONE;
	ComPtr<ID3D11RasterizerState> newRSState;
	HRESULT hr = m_d3dDevice->CreateRasterizerState(&rsDesc, newRSState.GetAddressOf());
	DX::ThrowIfFailed(hr);
	m_d3dContext->RSSetState(newRSState.Get());

	Super::Render();

	m_d3dContext->OMSetBlendState(nullptr, blendFactors, UINT_MAX);
	m_d3dContext->RSSetState(previousRSState.Get());
}

void Crate::BuildShape()
{
	// Set vertex buffer
	VertexType vertices[] =
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

	float scale = 5.f;
	float positionOffset = 3.f;

	int vertexCount = ARRAYSIZE(vertices);
	for (int i = 0; i < vertexCount; ++i)
	{
		vertices[i].position.x *= scale;
		vertices[i].position.y *= scale;
		vertices[i].position.z *= scale;

		vertices[i].position.y += positionOffset;
	}

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

	UINT stride = sizeof(VertexType);
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
	cbDesc.ByteWidth = sizeof(cbPerObjectStruct);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBufferPerObject.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void Crate::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_cbPerObject.material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
}

void Crate::BuildShader()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
#if ENABLEFOG
		"FOG", "1",
#endif
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const std::wstring shaderFilename = L"TransparentWaveGame\\Blending.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename, alphaTestDefines);
}

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV

void Crate::BuildTexture()
{
	BuildTextureByName(L"TransparentWaveGame\\WireFence.dds");
}

#endif