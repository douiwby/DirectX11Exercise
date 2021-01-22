#include "pch.h"
#include "MirrorGame/MirrorGame.h"
#include "Common/VertexStructuer.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using VertexType = VertexPositionNormalUV;

MirrorGame::~MirrorGame()
{
	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		if (*it)
		{
			delete (*it);
			*it = nullptr;
		}
	}
	if (mirror)
	{
		delete mirror;
		mirror = nullptr;
	}
	if (shadow)
	{
		delete shadow;
		shadow = nullptr;
	}

	m_objects.clear();
	m_reflectObjects.clear();
}

void MirrorGame::Initialize(HWND window, int width, int height)
{
	m_initCameraY = 20.f;
	m_initCameraZ = -20.f;
	m_maxRadius = 100.f;
	m_mouseMoveRate = 5.f;

	Super::Initialize(window, width, height);
	mirror->Initialize(m_d3dDevice, m_d3dContext, &m_view, &m_proj);
	shadow->Initialize(m_d3dDevice, m_d3dContext, &m_view, &m_proj);

	BuildLight();
}

void MirrorGame::BuildLight()
{
	// Directional light.
	m_dirLight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLight.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLight.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.4f);

	// Point light--position is changed every frame to animate in UpdateScene function.
	m_pointLight.Ambient = XMFLOAT4(0.3f, 0.3f, 0.3f, 1.0f);
	m_pointLight.Diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_pointLight.Specular = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_pointLight.Att = XMFLOAT3(0.0f, 0.1f, 0.0f);
	m_pointLight.Range = 25.0f;

	// Spot light--position and direction changed every frame to animate in UpdateScene function.
	m_spotLight.Ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_spotLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);
	m_spotLight.Specular = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_spotLight.Att = XMFLOAT3(1.0f, 0.0f, 0.0f);
	m_spotLight.Spot = 96.0f;
	m_spotLight.Range = 1000.0f;
	m_spotLight.Position = XMFLOAT3(0.0f, 15.0f, -3.0f);
	m_spotLight.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);

	m_cbPerFrame.dirLight = m_dirLight;
	m_cbPerFrame.pointLight = m_pointLight;
	m_cbPerFrame.spotLight = m_spotLight;

	m_dirLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_pointLight.Diffuse = XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	m_spotLight.Diffuse = XMFLOAT4(1.0f, 1.0f, 0.0f, 1.0f);

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

void MirrorGame::Update(DX::StepTimer const & timer)
{
	Super::Update(timer);

	UpdateLightPosition(timer);

	m_cbPerFrame.dirLight = m_dirLight;
	m_cbPerFrame.spotLight = m_spotLight;
	m_cbPerFrame.eyePosW = m_eyePos;
	m_d3dContext->UpdateSubresource(m_constantBufferPerFrame.Get(), 0, nullptr, &m_cbPerFrame, 0, 0);
}

void MirrorGame::UpdateLightPosition(DX::StepTimer const & timer)
{
	float elapsedTime = float(timer.GetElapsedSeconds());
	
	// Control the spot light by keyboard.
	// Key map
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes

	// Use keyboard to control the position
	float movingSpeed = 15.0f;
	if (GetAsyncKeyState(VK_LEFT) & 0x8000)
	{
		m_spotLight.Position.x -= movingSpeed * elapsedTime;
	}
	if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		m_spotLight.Position.x += movingSpeed * elapsedTime;

	if (GetAsyncKeyState(VK_UP) & 0x8000)
		m_spotLight.Position.z += movingSpeed * elapsedTime;

	if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		m_spotLight.Position.z -= movingSpeed * elapsedTime;

	float rotateSpeed = 45.f / 180.f * XM_PI;
 	if (GetAsyncKeyState(0x41) & 0x8000)  // A key
 	{
		float theta = -rotateSpeed * elapsedTime;
		RotateVectorByZAxis(m_spotLight.Direction, theta);
 	}
 	if (GetAsyncKeyState(0x44) & 0x8000)  // D key
 	{
		float theta = rotateSpeed * elapsedTime;
		RotateVectorByZAxis(m_spotLight.Direction, theta);
	}
	if (GetAsyncKeyState(0x57) & 0x8000)  // W key
	{
		float theta = -rotateSpeed * elapsedTime;
		RotateVectorByXAxis(m_spotLight.Direction, theta);
	}
	if (GetAsyncKeyState(0x53) & 0x8000)  // S key
	{
		float theta = rotateSpeed * elapsedTime;
		RotateVectorByXAxis(m_spotLight.Direction, theta);
	}
}

void MirrorGame::PreObjectsRender()
{
	mirror->Update(m_timer);
}

void MirrorGame::PostObjectsRender()
{
	// -----------------------------
	// Render the shadow of box
	// -----------------------------

	CD3D11_DEPTH_STENCIL_DESC noDoubleBlendDesc(D3D11_DEFAULT);
	noDoubleBlendDesc.StencilEnable = TRUE;
	noDoubleBlendDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;
	noDoubleBlendDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_INCR;

	ComPtr<ID3D11DepthStencilState> mShadowState;
	HRESULT hr = m_d3dDevice->CreateDepthStencilState(&noDoubleBlendDesc, mShadowState.GetAddressOf());
	DX::ThrowIfFailed(hr);

	CD3D11_BLEND_DESC shadowBlendDesc(D3D11_DEFAULT);
	shadowBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	shadowBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	shadowBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	shadowBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	shadowBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	shadowBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	shadowBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	ComPtr<ID3D11BlendState> mShadowtBlendState;
	hr = m_d3dDevice->CreateBlendState(&shadowBlendDesc, mShadowtBlendState.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_STENCIL, 1.0f, 0);
	m_d3dContext->OMSetDepthStencilState(mShadowState.Get(), 0);
	float blendFactorsShadow[] = { 0.f,0.f,0.f,1.f };
	m_d3dContext->OMSetBlendState(mShadowtBlendState.Get(), blendFactorsShadow, UINT_MAX);

	XMVECTOR shadowPlane = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	XMVECTOR toDirLight = -XMLoadFloat3(&m_dirLight.Direction);
	XMMATRIX shadowMatrix = XMMatrixShadow(shadowPlane, toDirLight);
	XMMATRIX shadowOffsetY = XMMatrixTranslation(0.f, 0.001f, 0.f);

	XMMATRIX origionalWorld = XMLoadFloat4x4(shadow->m_world);
	XMMATRIX shadowWorld = origionalWorld * shadowMatrix * shadowOffsetY;

	XMStoreFloat4x4(shadow->m_world, shadowWorld);

	shadow->Update(m_timer);
	shadow->Render();

	m_d3dContext->OMSetDepthStencilState(nullptr, 0);
	m_d3dContext->OMSetBlendState(nullptr, blendFactorsShadow, UINT_MAX);

	// -----------------------------
	// Render the mirror to get the stencil
	// -----------------------------

	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_STENCIL, 1.0f, 0);

	UINT stencilRef = 1;

	CD3D11_DEPTH_STENCIL_DESC mirrorDesc(D3D11_DEFAULT);
	mirrorDesc.DepthEnable = TRUE;
	mirrorDesc.StencilEnable = TRUE;
	mirrorDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_REPLACE;
	mirrorDesc.FrontFace.StencilFunc = D3D11_COMPARISON_ALWAYS;

	ComPtr<ID3D11DepthStencilState> mMirrorState;
	/*HRESULT*/ hr = m_d3dDevice->CreateDepthStencilState(&mirrorDesc, mMirrorState.GetAddressOf());
	DX::ThrowIfFailed(hr);
	m_d3dContext->OMSetDepthStencilState(mMirrorState.Get(), stencilRef);

	mirror->Render();

	// -----------------------------
	// Use stencil to render other objects
	// -----------------------------

	// Because all the objects is behind the wall, so close depth test here or they won't draw.
	CD3D11_DEPTH_STENCIL_DESC reflectObjecDesc(D3D11_DEFAULT);
	reflectObjecDesc.DepthEnable = FALSE;
	reflectObjecDesc.StencilEnable = TRUE;
	reflectObjecDesc.FrontFace.StencilPassOp = D3D11_STENCIL_OP_KEEP;
	reflectObjecDesc.FrontFace.StencilFunc = D3D11_COMPARISON_EQUAL;

	// But this will cause an issue:
	// The objects render later won't have depth test and overlap with each other,
	// and the blend enable will make them looks like transparent.
	// So clear the depth here to draw the reflect objects.
	// Note you shouldn't draw normal objects after this because depth information has lost already!!!
	reflectObjecDesc.DepthEnable = TRUE;
	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0f, 0);

	ComPtr<ID3D11DepthStencilState> mReflectObjectState;
	hr = m_d3dDevice->CreateDepthStencilState(&reflectObjecDesc, mReflectObjectState.GetAddressOf());
	DX::ThrowIfFailed(hr);
	m_d3dContext->OMSetDepthStencilState(mReflectObjectState.Get(), stencilRef);

	// The normal of triangles won't change after reflect,
	// so enable counter clockwise to cull back face correctly.
	ComPtr<ID3D11RasterizerState> previousState;
	m_d3dContext->RSGetState(previousState.GetAddressOf());

	CD3D11_RASTERIZER_DESC reflectRasterizerDesc(D3D11_DEFAULT);
	reflectRasterizerDesc.FrontCounterClockwise = TRUE;

	ComPtr<ID3D11RasterizerState> mReflectRasterizerState;
	hr = m_d3dDevice->CreateRasterizerState(&reflectRasterizerDesc, mReflectRasterizerState.GetAddressOf());
	DX::ThrowIfFailed(hr);
	m_d3dContext->RSSetState(mReflectRasterizerState.Get());

	// Reflect plane
	XMFLOAT4 plane(0.f, 0.f, -1.f, 0.f);
	XMVECTOR mirrorPlane = XMLoadFloat4(&plane);
	XMMATRIX reflectMatrix = XMMatrixReflect(mirrorPlane);

	// Reflect light
	XMVECTOR pos = XMLoadFloat3(&m_spotLight.Position);
	XMVECTOR reflectPos = XMVector3Transform(pos, reflectMatrix);
	XMStoreFloat3(&m_spotLight.Position, reflectPos);

	XMVECTOR dir = XMLoadFloat3(&m_spotLight.Direction);
	XMVECTOR reflectDir = XMVector3Transform(dir, reflectMatrix);
	XMStoreFloat3(&m_spotLight.Direction, reflectDir);

	m_cbPerFrame.spotLight = m_spotLight;
	m_d3dContext->UpdateSubresource(m_constantBufferPerFrame.Get(), 0, nullptr, &m_cbPerFrame, 0, 0);

	for (auto it = m_reflectObjects.begin(); it != m_reflectObjects.end(); ++it)
	{
		// Reflect objects
		XMMATRIX world = XMLoadFloat4x4((*it)->m_world);
		XMMATRIX reflectWorld = XMMatrixMultiply(world, reflectMatrix);
		XMStoreFloat4x4((*it)->m_world, reflectWorld);

		(*it)->Update(m_timer);
		(*it)->Render();

		XMStoreFloat4x4((*it)->m_world, world);
	}

	// Render the reflect shadow
	{	
		float blendFactorsShadow[] = { 0.f,0.f,0.f,1.f };
		m_d3dContext->OMSetBlendState(mShadowtBlendState.Get(), blendFactorsShadow, UINT_MAX);

		XMVECTOR shadowPlane = XMVectorSet(0.f, 1.f, 0.f, 0.f);
		XMVECTOR toDirLight = -XMLoadFloat3(&m_dirLight.Direction);
		XMMATRIX shadowMatrix = XMMatrixShadow(shadowPlane, toDirLight);
		XMMATRIX shadowOffsetY = XMMatrixTranslation(0.f, 0.001f, 0.f);

		XMMATRIX origionalWorld = XMLoadFloat4x4(shadow->m_world);
		XMMATRIX shadowWorld = origionalWorld * shadowMatrix * shadowOffsetY;

		XMStoreFloat4x4(shadow->m_world, shadowWorld);

		// Reflect objects
		XMMATRIX world = XMLoadFloat4x4(shadow->m_world);
		XMMATRIX reflectWorld = XMMatrixMultiply(world, reflectMatrix);
		XMStoreFloat4x4(shadow->m_world, reflectWorld);

		shadow->Update(m_timer);
		shadow->Render();

		m_d3dContext->OMSetBlendState(nullptr, blendFactorsShadow, UINT_MAX);
	}

	m_d3dContext->RSSetState(previousState.Get());

	// Restore the shadow's world matrix
	XMStoreFloat4x4(shadow->m_world, origionalWorld);

	// -----------------------------
	// Render the mirror normally at last
	// -----------------------------

	// Enable blend to render the object
	CD3D11_BLEND_DESC reflectBlendDesc(D3D11_DEFAULT);
	reflectBlendDesc.RenderTarget[0].BlendEnable = TRUE;
	//reflectBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_BLEND_FACTOR;
	//reflectBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_BLEND_FACTOR;
	reflectBlendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA;
	reflectBlendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	reflectBlendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
	reflectBlendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
	reflectBlendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ZERO;
	reflectBlendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;

	ComPtr<ID3D11BlendState> mReflectBlendState;
	hr = m_d3dDevice->CreateBlendState(&reflectBlendDesc, mReflectBlendState.GetAddressOf());
	DX::ThrowIfFailed(hr);
	float blendFactors[] = { 0.15f,0.15f,0.15f,1.f };
	m_d3dContext->OMSetBlendState(mReflectBlendState.Get(), blendFactors, UINT_MAX);

	mirror->Render();

	// Restore the light
	XMStoreFloat3(&m_spotLight.Direction, dir);
	XMStoreFloat3(&m_spotLight.Position, pos);

	// Clear the state
	m_d3dContext->OMSetDepthStencilState(nullptr, 0);
	m_d3dContext->OMSetBlendState(nullptr, blendFactors, UINT_MAX);
}

void MirrorGame::AddObjects()
{
	ReflectShape* wall = new Wall();
	ReflectShape* floor = new Floor();
	ReflectShape* crate = new LitCrate();
	m_objects.push_back(wall);
	m_objects.push_back(floor);
	m_objects.push_back(crate);
	m_reflectObjects.push_back(floor);
	m_reflectObjects.push_back(crate);
	mirror = new Mirror();
	shadow = new LitCrateShadow();
}

inline void MirrorGame::RotateVectorByZAxis(DirectX::XMFLOAT3& vector, float rotateRadian)
{
	XMFLOAT4X4 rotation =
	{
		cosf(rotateRadian),sinf(rotateRadian),0.f,0.f,
		-sinf(rotateRadian),cosf(rotateRadian),0.f,0.f,
		0.f,0.f,1.f,0.f,
		0.f,0.f,0.f,1.f
	};
	RotateVector(vector, rotation);
}

inline void MirrorGame::RotateVectorByXAxis(DirectX::XMFLOAT3 & vector, float rotateRadian)
{
	XMFLOAT4X4 rotation =
	{
		1.f,0.f,0.f,0.f,
		0.f,cosf(rotateRadian),sinf(rotateRadian),0.f,
		0.f,-sinf(rotateRadian),cosf(rotateRadian),0.f,
		0.f,0.f,0.f,1.f
	};
	RotateVector(vector, rotation);
}

inline void MirrorGame::RotateVector(DirectX::XMFLOAT3& vector, DirectX::XMFLOAT4X4& rotation)
{
	XMVECTOR dir = XMLoadFloat3(&vector);
	dir = XMVector3Transform(dir, XMLoadFloat4x4(&rotation));
	XMStoreFloat3(&vector, dir);
}

void Wall::Render()
{
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

	m_d3dContext->PSSetSamplers(0, 1, previousSamplerState.GetAddressOf());
}

void Wall::BuildShape()
{
	// Vertex buffer

	VertexType vertices[] =
	{
		{XMFLOAT3(-width/2, height, 0.f),XMFLOAT3(0.f,0.f,-1.f),XMFLOAT2(0.f,0.f)},
		{XMFLOAT3( width/2, height, 0.f),XMFLOAT3(0.f,0.f,-1.f),XMFLOAT2(width,0.f)},
		{XMFLOAT3(-width/2, 0.f, 0.f),XMFLOAT3(0.f,0.f,-1.f),XMFLOAT2(0.f,height)},
		{XMFLOAT3( width/2, 0.f, 0.f),XMFLOAT3(0.f,0.f,-1.f),XMFLOAT2(width,height)}
	};

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = ARRAYSIZE(vertices) * sizeof(VertexType);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vbData;
	vbData.pSysMem = vertices;
	vbData.SysMemPitch = 0;
	vbData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Index buffer

	UINT indices[] =
	{
		0,1,2,
		2,1,3
	};

	m_indexCount = ARRAYSIZE(indices);

	CD3D11_BUFFER_DESC ibDesc(m_indexCount * sizeof(UINT), D3D11_BIND_INDEX_BUFFER);

	D3D11_SUBRESOURCE_DATA ibData;
	ibData.pSysMem = indices;
	ibData.SysMemPitch = 0;
	ibData.SysMemSlicePitch = 0;

	hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void Wall::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	m_cbPerObject.material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
}

void Wall::BuildShader()
{
	const std::wstring shaderFilename = L"TransparentWaveGame\\Blending.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename);
}

void Wall::BuildTexture()
{
	BuildTextureByName(L"MirrorGame\\brick01.dds");
}

void Floor::BuildShape()
{
	// Vertex buffer

	VertexType vertices[] =
	{
		{XMFLOAT3(-width/2, 0.f, height/2),XMFLOAT3(0.f,1.f,0.f),XMFLOAT2(0.f,0.f)} ,
		{XMFLOAT3( width/2, 0.f, height/2),XMFLOAT3(0.f,1.f,0.f),XMFLOAT2(width,0.f)},
		{XMFLOAT3(-width/2, 0.f,-height/2),XMFLOAT3(0.f,1.f,0.f),XMFLOAT2(0.f,height)},
		{XMFLOAT3( width/2, 0.f,-height/2),XMFLOAT3(0.f,1.f,0.f),XMFLOAT2(width,height)}
	};

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = ARRAYSIZE(vertices) * sizeof(VertexType);
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vbData;
	vbData.pSysMem = vertices;
	vbData.SysMemPitch = 0;
	vbData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Index buffer

	UINT indices[] =
	{
		0,1,2,
		2,1,3
	};

	m_indexCount = ARRAYSIZE(indices);

	CD3D11_BUFFER_DESC ibDesc(m_indexCount * sizeof(UINT), D3D11_BIND_INDEX_BUFFER);

	D3D11_SUBRESOURCE_DATA ibData;
	ibData.pSysMem = indices;
	ibData.SysMemPitch = 0;
	ibData.SysMemSlicePitch = 0;

	hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibData, m_indexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	XMMATRIX world = XMLoadFloat4x4(m_world);
	world = XMMatrixMultiply(world, XMMatrixTranslation(0.f, 0.f, -height / 2));
	XMStoreFloat4x4(m_world, world);
}

void Floor::BuildTexture()
{
	BuildTextureByName(L"MirrorGame\\checkboard.dds");
}

void LitCrate::BuildShape()
{
	float halfEdgeLength = edgeLength / 2;

	// Set vertex buffer
	VertexType vertices[] =
	{
		// Front face
		{ XMFLOAT3(-halfEdgeLength, +halfEdgeLength, -halfEdgeLength), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(+halfEdgeLength, +halfEdgeLength, -halfEdgeLength), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(2.f,0.f) },
		{ XMFLOAT3(-halfEdgeLength, -halfEdgeLength, -halfEdgeLength), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(0.f,2.f) },
		{ XMFLOAT3(+halfEdgeLength, -halfEdgeLength, -halfEdgeLength), XMFLOAT3(0.0f, 0.0f, -1.0f), XMFLOAT2(2.f,2.f) },
		// Back face
		{ XMFLOAT3(+halfEdgeLength, +halfEdgeLength, +halfEdgeLength), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(-halfEdgeLength, +halfEdgeLength, +halfEdgeLength), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(+halfEdgeLength, -halfEdgeLength, +halfEdgeLength), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(-halfEdgeLength, -halfEdgeLength, +halfEdgeLength), XMFLOAT3(0.0f, 0.0f, 1.0f), XMFLOAT2(1.f,1.f) },
		// Left face
		{ XMFLOAT3(-halfEdgeLength, +halfEdgeLength, +halfEdgeLength), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(-halfEdgeLength, +halfEdgeLength, -halfEdgeLength), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(-halfEdgeLength, -halfEdgeLength, +halfEdgeLength), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(-halfEdgeLength, -halfEdgeLength, -halfEdgeLength), XMFLOAT3(-1.0f, 0.0f, 0.0f), XMFLOAT2(1.f,1.f) },
		// Right face
		{ XMFLOAT3(+halfEdgeLength, +halfEdgeLength, -halfEdgeLength), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(+halfEdgeLength, +halfEdgeLength, +halfEdgeLength), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(+halfEdgeLength, -halfEdgeLength, -halfEdgeLength), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(+halfEdgeLength, -halfEdgeLength, +halfEdgeLength), XMFLOAT3(1.0f, 0.0f, 0.0f), XMFLOAT2(1.f,1.f) },
		// Top face
		{ XMFLOAT3(-halfEdgeLength, +halfEdgeLength, +halfEdgeLength), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(+halfEdgeLength, +halfEdgeLength, +halfEdgeLength), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(-halfEdgeLength, +halfEdgeLength, -halfEdgeLength), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(+halfEdgeLength, +halfEdgeLength, -halfEdgeLength), XMFLOAT3(0.0f, 1.0f, 0.0f), XMFLOAT2(1.f,1.f) },
		// Bottom face
		{ XMFLOAT3(-halfEdgeLength, -halfEdgeLength, -halfEdgeLength), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.f,0.f) },
		{ XMFLOAT3(+halfEdgeLength, -halfEdgeLength, -halfEdgeLength), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.f,0.f) },
		{ XMFLOAT3(-halfEdgeLength, -halfEdgeLength, +halfEdgeLength), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.f,1.f) },
		{ XMFLOAT3(+halfEdgeLength, -halfEdgeLength, +halfEdgeLength), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(1.f,1.f) }
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

	UINT stride = sizeof(VertexType);
	UINT offset = 0;

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

	XMMATRIX world = XMLoadFloat4x4(m_world);
	world = XMMatrixMultiply(world, XMMatrixTranslation(0.f, halfEdgeLength, -3.f));
	XMStoreFloat4x4(m_world, world);
}

void LitCrate::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(0.7f, 0.7f, 0.7f, 1.0f);
	m_cbPerObject.material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
}

void LitCrate::BuildShader()
{
	const std::wstring shaderFilename = L"TransparentWaveGame\\Blending.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename);
}

void LitCrate::BuildTexture()
{
	BuildTextureByName(L"CrateGame\\WoodCrate01.dds");
}

void Mirror::BuildShape()
{
	Super::BuildShape();

	XMMATRIX world = XMLoadFloat4x4(m_world);
	world = XMMatrixMultiply(world, XMMatrixTranslation(0.f, 0.f, -0.001f));
	XMStoreFloat4x4(m_world, world);
}

void Mirror::BuildTexture()
{
	BuildTextureByName(L"MirrorGame\\ice.dds");
}

void Mirror::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 0.15f);
	m_cbPerObject.material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
}

void LitCrateShadow::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.5f);
	m_cbPerObject.material.specular = XMFLOAT4(0.0f, 0.0f, 0.0f, 16.0f);
}
