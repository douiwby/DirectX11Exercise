#include "pch.h"
#include "TreeBillboardGame/TreeBillboardGame.h"
#include "DDSTextureLoader.h"
#include <ctime>

#define ENABLEFOG 0

struct SquareBillboardVertexType
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT2 size;
};

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using VertexType = SquareBillboardVertexType;

void TreeBillboardGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	m_initCameraY = 50.f;
	m_initCameraZ = -50.f;

	m_d3dContext->GSSetConstantBuffers(0, 1, m_constantBufferPerFrame.GetAddressOf());
}

void TreeBillboardGame::AddObjects()
{
	m_objects.push_back(new TreeBillboard());

	Super::AddObjects();
}

void TreeBillboard::BuildShader()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
#if ENABLEFOG
		"FOG", "1",
#endif
		"ALPHA_TEST", "1",
		NULL, NULL
	};

	const std::wstring shaderFilename = L"TreeBillboardGame\\TreeSprite.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename, alphaTestDefines);

	m_GSByteCode = d3dUtil::CompileShader(shaderFilename, alphaTestDefines, "GS", "gs_5_0");

	HRESULT hr = m_d3dDevice->CreateGeometryShader(m_GSByteCode->GetBufferPointer(), m_GSByteCode->GetBufferSize(), nullptr, m_geometryShader.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void TreeBillboard::SetInputLayout()
{
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"SIZE",0,DXGI_FORMAT_R32G32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
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

inline float TreeBillboard::GetHeight(float x, float z) const
{
	return 0.3f*(z*sinf(0.1f*x) + x * cosf(0.1f*z));
}

void TreeBillboard::BuildShape()
{
	VertexType vertices[m_treeCount];

	time_t t;
	srand((unsigned)time(&t));

	float halfRange = m_ForestHalfRange;
	for (UINT i = 0; i < m_treeCount; ++i)
	{
		float x = -halfRange + ((float)(rand()) / (float)RAND_MAX) * halfRange * 2;
		float z = -halfRange + ((float)(rand()) / (float)RAND_MAX) * halfRange * 2;
		float y = GetHeight(x, z);

		// Move tree slightly above land height.
		y += 10.0f;

		vertices[i].position = XMFLOAT3(x, y, z);
		vertices[i].size = XMFLOAT2(m_BillboardWidth, m_BillboardHeight);
	}

	D3D11_BUFFER_DESC vbd;
	vbd.Usage = D3D11_USAGE_IMMUTABLE;
	vbd.ByteWidth = sizeof(VertexType) * m_treeCount;
	vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbd.CPUAccessFlags = 0;
	vbd.MiscFlags = 0;
	D3D11_SUBRESOURCE_DATA vinitData;
	vinitData.pSysMem = vertices;
	vinitData.SysMemPitch = 0;
	vinitData.SysMemSlicePitch = 0;
	HRESULT hr =  m_d3dDevice->CreateBuffer(&vbd, &vinitData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void TreeBillboard::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_cbPerObject.material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
}

void TreeBillboard::BuildTexture()
{
#define USING_ARRAY_TEXTURE 0
#if USING_ARRAY_TEXTURE
	ComPtr<ID3D11Texture2D> srcTextures;
	ComPtr<ID3D11Resource> textureResources;

	const wchar_t * fileName = L"TreeBillboardGame\\treearray.dds";
	HRESULT hr = CreateDDSTextureFromFile(m_d3dDevice.Get(), fileName, textureResources.GetAddressOf(), m_diffuseMapView.GetAddressOf());
	DX::ThrowIfFailed(hr);

	hr = textureResources.As(&srcTextures);
	DX::ThrowIfFailed(hr);

	D3D11_TEXTURE2D_DESC texElementDesc;
	srcTextures->GetDesc(&texElementDesc);
	UINT arraySize = texElementDesc.ArraySize;  // Can send to shader
#else
	std::vector<std::wstring> treeFilenames;
	//treeFilenames.push_back(L"TreeBillboardGame/tree0.dds"); // This texture's format is different with other 3
	treeFilenames.push_back(L"TreeBillboardGame/tree1.dds");
	treeFilenames.push_back(L"TreeBillboardGame/tree2.dds");
	treeFilenames.push_back(L"TreeBillboardGame/tree3.dds");

	int texArraySize = 3;

	std::vector<ComPtr<ID3D11Texture2D>> srcTextures(texArraySize);
	std::vector<ComPtr<ID3D11Resource>> textureResources(texArraySize);

	for (int i = 0; i < texArraySize; ++i)
	{
		HRESULT hr = CreateDDSTextureFromFileEx(m_d3dDevice.Get(), treeFilenames[i].c_str(),
			0, D3D11_USAGE_STAGING, 0, D3D11_CPU_ACCESS_READ, 0, false,
			textureResources[i].GetAddressOf(), nullptr);
		DX::ThrowIfFailed(hr);

		hr = textureResources[i].As(&srcTextures[i]);
		DX::ThrowIfFailed(hr);

		D3D11_TEXTURE2D_DESC texElementDesc;
		srcTextures[i]->GetDesc(&texElementDesc);

		DXGI_FORMAT format = texElementDesc.Format;
	}

	D3D11_TEXTURE2D_DESC texArrayDesc;
	srcTextures[0]->GetDesc(&texArrayDesc);
	texArrayDesc.ArraySize = texArraySize;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;

	ComPtr<ID3D11Texture2D> texArray;
	HRESULT hr = m_d3dDevice->CreateTexture2D(&texArrayDesc, nullptr, texArray.GetAddressOf());
	DX::ThrowIfFailed(hr);

	for (int i = 0; i < texArraySize; ++i)
	{
		for (int mipLevel = 0; mipLevel < texArrayDesc.MipLevels; ++mipLevel)
		{
			D3D11_MAPPED_SUBRESOURCE mappedTex2D;
			hr = m_d3dContext->Map(srcTextures[i].Get(), mipLevel, D3D11_MAP_READ, 0, &mappedTex2D);
			DX::ThrowIfFailed(hr);

			m_d3dContext->UpdateSubresource(
				texArray.Get(),
				D3D11CalcSubresource(mipLevel, i, texArrayDesc.MipLevels),
				0,
				mappedTex2D.pData,
				mappedTex2D.RowPitch,
				mappedTex2D.DepthPitch
			);

			m_d3dContext->Unmap(srcTextures[i].Get(), mipLevel);
		}
	}
	
	D3D11_SHADER_RESOURCE_VIEW_DESC texArraySRVDesc;
	texArraySRVDesc.Format = texArrayDesc.Format;
	texArraySRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
	texArraySRVDesc.Texture2DArray.MostDetailedMip = 0;
	texArraySRVDesc.Texture2DArray.MipLevels = texArrayDesc.MipLevels;
	texArraySRVDesc.Texture2DArray.FirstArraySlice = 0;
	texArraySRVDesc.Texture2DArray.ArraySize = texArraySize;

	hr = m_d3dDevice->CreateShaderResourceView(texArray.Get(), &texArraySRVDesc, m_diffuseMapView.GetAddressOf());
	DX::ThrowIfFailed(hr);
#endif
}

void TreeBillboard::Update(DX::StepTimer const & timer)
{
	Super::Update(timer);

	if (GetAsyncKeyState('R') & 0x8000)
		m_AlphaToCoverageOn = true;

	if (GetAsyncKeyState('T') & 0x8000)
		m_AlphaToCoverageOn = false;
}

void TreeBillboard::Render()
{
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(VertexType);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_POINTLIST);

	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->VSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());

	m_d3dContext->GSSetShader(m_geometryShader.Get(), nullptr, 0);
	m_d3dContext->GSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());

	m_d3dContext->PSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());
	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	m_d3dContext->PSSetShaderResources(0, 1, m_diffuseMapView.GetAddressOf());

	float blendFactors[] = { 0.f,0.f,0.f,0.f };
	if (m_AlphaToCoverageOn)
	{
		CD3D11_BLEND_DESC bd(D3D11_DEFAULT);
		bd.AlphaToCoverageEnable = TRUE;
		ComPtr<ID3D11BlendState> mBlendState;
		m_d3dDevice->CreateBlendState(&bd, mBlendState.GetAddressOf());
		m_d3dContext->OMSetBlendState(mBlendState.Get(), blendFactors, UINT_MAX);
	}

	m_d3dContext->Draw(m_treeCount, 0);

	m_d3dContext->GSSetShader(nullptr, nullptr, 0);
	m_d3dContext->OMSetBlendState(nullptr, blendFactors, UINT_MAX);
}