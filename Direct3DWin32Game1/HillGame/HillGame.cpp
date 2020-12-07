#include "pch.h"
#include "HillGame.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void HillGame::Initialize(HWND window, int width, int height)
{
	m_initCameraY = 150.f;
	m_initCameraZ = -150.f;
	m_maxRadius = 300.f;
	m_mouseMoveRate = 15.f;

	Super::Initialize(window, width, height);

	SetInputLayout();
	BuildHill();

}

void HillGame::SetInputLayout()
{
	ComPtr<ID3DBlob> mvsByteCode = d3dUtil::CompileShader(L"HillGame\\Hill.hlsl", nullptr, "VS", "vs_5_0");
	ComPtr<ID3DBlob> mpsByteCode = d3dUtil::CompileShader(L"HillGame\\Hill.hlsl", nullptr, "PS", "ps_5_0");

	HRESULT hr = m_d3dDevice->CreateVertexShader(mvsByteCode->GetBufferPointer(), mvsByteCode->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreatePixelShader(mpsByteCode->GetBufferPointer(), mpsByteCode->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
	DX::ThrowIfFailed(hr);

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,24,D3D11_INPUT_PER_VERTEX_DATA,0}
	};

	hr = m_d3dDevice->CreateInputLayout(
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

inline float HillGame::GetHeight(float x, float z) const
{
	return 0.3f*(z*sinf(0.1f*x) + x * cosf(0.1f*z));
}

void HillGame::BuildHill()
{
	// CreateGrid
	float width = 150.f;
	float depth = 150.f;
	const UINT m = 50;
	const UINT n = 50;

	const UINT vertexCount = m * n;
	const UINT faceCount = (m - 1)*(n - 1) * 2;

	//
	// Create the vertices.
	//

	float halfWidth = 0.5f*width;
	float halfDepth = 0.5f*depth;

	float dx = width / (n - 1);
	float dz = depth / (m - 1);

	float du = 1.0f / (n - 1);
	float dv = 1.0f / (m - 1);

	HillGameVertex vertices[vertexCount];

	for (UINT i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dz;
		for (UINT j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;
			float y = GetHeight(x, z);

			vertices[i*n + j].position = XMFLOAT3(x, y, z);
			
			XMFLOAT4 color;
			if (y < -10.0f)
			{
				// Sandy beach color.
				color = XMFLOAT4(1.0f, 0.96f, 0.62f, 1.0f);
			}
			else if (y < 5.0f)
			{
				// Light yellow-green.
				color = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
			}
			else if (y < 12.0f)
			{
				// Dark yellow-green.
				color = XMFLOAT4(0.1f, 0.48f, 0.19f, 1.0f);
			}
			else if (y < 20.0f)
			{
				// Dark brown.
				color = XMFLOAT4(0.45f, 0.39f, 0.34f, 1.0f);
			}
			else
			{
				// White snow.
				color = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
			}
			vertices[i*n + j].color = color;

			vertices[i*n + j].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
		}
	}

	//
	// Create the indices.
	//

	UINT indices[faceCount * 3];

	// Iterate over each quad and compute indices.
	UINT k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (UINT j = 0; j < n - 1; ++j)
		{
			indices[k] = i * n + j;
			indices[k + 1] = i * n + j + 1;
			indices[k + 2] = (i + 1)*n + j;

			indices[k + 3] = (i + 1)*n + j;
			indices[k + 4] = i * n + j + 1;
			indices[k + 5] = (i + 1)*n + j + 1;

			k += 6; // next quad
		}
	}

	// Vertex buffer

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

	UINT stride = sizeof(HillGameVertex);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);

	// Index buffer

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
	DX::ThrowIfFailed(hr);

	m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBuffer.GetAddressOf());
}

// Updates the world.
void HillGame::Update(DX::StepTimer const& timer)
{
	Super::Update(timer);

	XMMATRIX world = XMLoadFloat4x4(&m_world);
	XMMATRIX view = XMLoadFloat4x4(&m_view);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);

	XMMATRIX mWorldViewProj = XMMatrixMultiply(XMMatrixMultiply(world, view), proj);
	XMFLOAT4X4 cbWorldViewProj;
	// Use XMMatrixTranspose before send to GPU due to HLSL using column-major
	XMStoreFloat4x4(&cbWorldViewProj, XMMatrixTranspose(mWorldViewProj));

	m_d3dContext->UpdateSubresource(m_constantBuffer.Get(), 0, nullptr, &cbWorldViewProj, 0, 0);
}

// Draws the scene.
void HillGame::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	Clear();

	m_d3dContext->DrawIndexed(m_indexCount, 0, 0);

	Present();
}