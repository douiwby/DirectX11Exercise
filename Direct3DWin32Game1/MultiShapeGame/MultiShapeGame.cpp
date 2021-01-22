#include "pch.h"
#include "MultiShapeGame/MultiShapeGame.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

MultiShapeGame::~MultiShapeGame()
{
	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		delete (*it);
		*it = nullptr;
	}
}

void MultiShapeGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);
}

void MultiShapeGame::AddObjects()
{
	m_objects.push_back(new Box());
	m_objects.push_back(new Box2());
	m_objects.push_back(new Pyramid());
}

void Box::Initialize(
	Microsoft::WRL::ComPtr<ID3D11Device>& device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
	DirectX::XMFLOAT4X4* view,
	DirectX::XMFLOAT4X4* proj)
{
	Super::Initialize(device, context, view, proj);

	SetInputLayout();
	BuildShape();
}

void Box::SetInputLayout()
{
	ComPtr<ID3DBlob> mvsByteCode = d3dUtil::CompileShader(L"BoxGame\\Base.hlsl", nullptr, "VS", "vs_5_0");
	ComPtr<ID3DBlob> mpsByteCode = d3dUtil::CompileShader(L"BoxGame\\Base.hlsl", nullptr, "PS", "ps_5_0");

	HRESULT hr = m_d3dDevice->CreateVertexShader(mvsByteCode->GetBufferPointer(), mvsByteCode->GetBufferSize(), nullptr, m_vertexShader.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreatePixelShader(mpsByteCode->GetBufferPointer(), mpsByteCode->GetBufferSize(), nullptr, m_pixelShader.GetAddressOf());
	DX::ThrowIfFailed(hr);

	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0},
		{"COLOR",0,DXGI_FORMAT_R32G32B32A32_FLOAT,0,12,D3D11_INPUT_PER_VERTEX_DATA,0}
	};

	hr = m_d3dDevice->CreateInputLayout(
		vertexDesc,
		ARRAYSIZE(vertexDesc),
		mvsByteCode->GetBufferPointer(),
		mvsByteCode->GetBufferSize(),
		m_inputLayout.GetAddressOf()
	);
	DX::ThrowIfFailed(hr);
}

void Box::BuildShape()
{
	// Set vertex buffer
	BoxGameVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) }
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

	// Set index buffer
	UINT indices[] =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
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

	// Set constant buffer
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(XMFLOAT4X4);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBufferPerObject.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

// Updates the world.
void Box::Update(DX::StepTimer const& timer)
{
	XMMATRIX world = XMLoadFloat4x4(m_world);
	XMMATRIX view = XMLoadFloat4x4(m_view);
	XMMATRIX proj = XMLoadFloat4x4(m_proj);

	world = XMMatrixMultiply(world, XMMatrixTranslation(2.5f, 0.f, 0.f));
	XMMATRIX mWorldViewProj = XMMatrixMultiply(XMMatrixMultiply(world, view), proj);
	XMFLOAT4X4 cbWorldViewProj;
	// Use XMMatrixTranspose before send to GPU due to HLSL using column-major
	XMStoreFloat4x4(&cbWorldViewProj, XMMatrixTranspose(mWorldViewProj));

	m_d3dContext->UpdateSubresource(m_constantBufferPerObject.Get(), 0, nullptr, &cbWorldViewProj, 0, 0);
}

// Draws the scene.
void Box::Render()
{

	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(BoxGameVertex);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBufferPerObject.GetAddressOf());
	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);

	m_d3dContext->DrawIndexed(m_indexCount, 0, 0);
}

void Box2::BuildShape()
{
	// Set vertex buffer
	BoxGameVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Magenta) },
		{ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) },
		{ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::White) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Cyan) },
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Black) },
		{ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) },
		{ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Blue) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) }
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

	// Set index buffer
	UINT indices[] =
	{
		// front face
		0, 1, 2,
		0, 2, 3,

		// back face
		4, 6, 5,
		4, 7, 6,

		// left face
		4, 5, 1,
		4, 1, 0,

		// right face
		3, 2, 6,
		3, 6, 7,

		// top face
		1, 5, 6,
		1, 6, 2,

		// bottom face
		4, 0, 3,
		4, 3, 7
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

	// Set constant buffer
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(XMFLOAT4X4);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBufferPerObject.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void Box2::Update(DX::StepTimer const & timer)
{
	XMMATRIX world = XMLoadFloat4x4(m_world);
	XMMATRIX view = XMLoadFloat4x4(m_view);
	XMMATRIX proj = XMLoadFloat4x4(m_proj);

	world = XMMatrixMultiply(world, XMMatrixTranslation(-2.5f, 0.f, 0.f));
	XMMATRIX mWorldViewProj = XMMatrixMultiply(XMMatrixMultiply(world, view), proj);
	XMFLOAT4X4 cbWorldViewProj;
	// Use XMMatrixTranspose before send to GPU due to HLSL using column-major
	XMStoreFloat4x4(&cbWorldViewProj, XMMatrixTranspose(mWorldViewProj));

	m_d3dContext->UpdateSubresource(m_constantBufferPerObject.Get(), 0, nullptr, &cbWorldViewProj, 0, 0);
}

void Pyramid::BuildShape()
{
	// Set vertex buffer
	BoxGameVertex vertices[] =
	{
		{ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::White) },
		{ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Green) },
		{ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Blue) },
		{ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Magenta) },
		{ XMFLOAT3(+0.0f, +1.5f, +0.0f), XMFLOAT4(Colors::Cyan) }
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

	// Set index buffer
	UINT indices[] =
	{
		// bottom face
		0, 2, 1,
		2, 3, 1,

		// side face
		4,3,2,
		4,1,3,
		4,0,1,
		4,2,0

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

	// Set constant buffer
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(XMFLOAT4X4);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBufferPerObject.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

// Updates the world.
void Pyramid::Update(DX::StepTimer const& timer)
{
	XMMATRIX world = XMLoadFloat4x4(m_world);
	XMMATRIX view = XMLoadFloat4x4(m_view);
	XMMATRIX proj = XMLoadFloat4x4(m_proj);

	world = XMMatrixMultiply(world, XMMatrixTranslation(0.f, 0.f, 0.f));
	XMMATRIX mWorldViewProj = XMMatrixMultiply(XMMatrixMultiply(world, view), proj);
	XMFLOAT4X4 cbWorldViewProj;
	// Use XMMatrixTranspose before send to GPU due to HLSL using column-major
	XMStoreFloat4x4(&cbWorldViewProj, XMMatrixTranspose(mWorldViewProj));

	m_d3dContext->UpdateSubresource(m_constantBufferPerObject.Get(), 0, nullptr, &cbWorldViewProj, 0, 0);
}