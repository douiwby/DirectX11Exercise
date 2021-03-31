#include "pch.h"
#include "SkyGame/SkyGame.h"
#include "Common/GeometryGenerator.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct VertexPosition
{
	DirectX::XMFLOAT3 position;
};

void SkyGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	if (m_reflectObject && m_reflectObject->bUsingDynamicCubeMap)
	{
		BuildDynamicCubeMapViews();
	}
}

void SkyGame::Update(DX::StepTimer const & timer)
{
	Super::Update(timer);
}

void SkyGame::AddObjects()
{
	Super::AddObjects();

	m_objects.push_back(new SkyBox());
	//m_objects.push_back(new SkySphere());

	//m_reflectObject = new ReflectBox();
	m_reflectObject = new ReflectSphere();
	m_objects.push_back(m_reflectObject);

	m_reflectObject->bUsingDynamicCubeMap = true;
}

void SkyGame::PreObjectsRender()
{
	m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBufferPerFrame.GetAddressOf());

	if (m_reflectObject && m_reflectObject->bUsingDynamicCubeMap)
	{
		RenderDynamicCubeMapTexture();
	}
}

void SkyGame::PostObjectsRender()
{
	if (m_reflectObject && m_reflectObject->bUsingDynamicCubeMap)
	{
		m_reflectObject->RenderInternal(m_dynamicCubeMapSRV);
	}
}

void SkyGame::BuildDynamicCubeMapViews()
{
	// Create Texture

	CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_R8G8B8A8_UNORM,m_cubeMapSize,m_cubeMapSize);
	texDesc.ArraySize = 6;
	texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
	texDesc.MiscFlags = D3D11_RESOURCE_MISC_TEXTURECUBE | D3D11_RESOURCE_MISC_GENERATE_MIPS;

	HRESULT hr = m_d3dDevice->CreateTexture2D(&texDesc, nullptr, m_dynamicCubeMapTexture.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Create Shader Resource View

	CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURECUBE);
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;

	hr = m_d3dDevice->CreateShaderResourceView(m_dynamicCubeMapTexture.Get(), &srvDesc, m_dynamicCubeMapSRV.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Create Render Target View

	CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2DARRAY);
	rtvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	rtvDesc.Texture2DArray.ArraySize = 1;
	for (int i = 0; i < 6; ++i)
	{
		rtvDesc.Texture2DArray.FirstArraySlice = i;

		hr = m_d3dDevice->CreateRenderTargetView(m_dynamicCubeMapTexture.Get(), &rtvDesc, m_dynamicCubeMapRTV[i].GetAddressOf());
		DX::ThrowIfFailed(hr);
	}

	// Create Depth Stencil View

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = m_cubeMapSize;
	depthStencilDesc.Height = m_cubeMapSize;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> mDepthStencilBuffer;
	hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, mDepthStencilBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D24_UNORM_S8_UINT);

	hr = m_d3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), &dsvDesc, m_dynamicCubeMapDSV.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void SkyGame::RenderDynamicCubeMapTexture()
{
	CD3D11_VIEWPORT dynamicCubeMapViewport(0.f, 0.f, m_cubeMapSize, m_cubeMapSize);
	m_d3dContext->RSSetViewports(1, &dynamicCubeMapViewport);

	XMMATRIX originalView = XMLoadFloat4x4(&m_view);
	XMMATRIX originalProj = XMLoadFloat4x4(&m_proj);

	// Use a 90 degree camera to cover all the scene
	XMMATRIX proj = XMMatrixPerspectiveFovLH(0.5*XM_PI, 1.f, m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_proj, proj);

	for (int i = 0; i < 6; ++i)
	{
		XMMATRIX view = XMLoadFloat4x4(&m_view);
		GenerateViewMatrixByIndex(m_reflectObject->m_world->_41, m_reflectObject->m_world->_42, m_reflectObject->m_world->_43, i, view);

		XMStoreFloat4x4(&m_view, view);
		XMStoreFloat4x4(&m_cbPerFrame.viewProj, XMMatrixTranspose(XMMatrixMultiply(view, proj)));
		d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerFrame, m_cbPerFrame);

		for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
		{
			Shape* shapeObject = dynamic_cast<Shape*>(*it);
			if (shapeObject)
			{
				shapeObject->UpdateConstantBufferPerObject();
			}
		}

		m_d3dContext->ClearRenderTargetView(m_dynamicCubeMapRTV[i].Get(), Colors::CornflowerBlue);
		m_d3dContext->ClearDepthStencilView(m_dynamicCubeMapDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
		m_d3dContext->OMSetRenderTargets(1, m_dynamicCubeMapRTV[i].GetAddressOf(), m_dynamicCubeMapDSV.Get());

		for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
		{
			(*it)->Render();
		}
	}

	// Have hardware generate lower mipmap levels of cube map.
	m_d3dContext->GenerateMips(m_dynamicCubeMapSRV.Get());

	// Reset the render target to back buffer
	Clear();

	CD3D11_VIEWPORT viewport(0.f, 0.f, m_outputWidth, m_outputHeight);
	m_d3dContext->RSSetViewports(1, &viewport);

	XMStoreFloat4x4(&m_view, originalView);
	XMStoreFloat4x4(&m_proj, originalProj);
	XMStoreFloat4x4(&m_cbPerFrame.viewProj, XMMatrixTranspose(XMMatrixMultiply(originalView, originalProj)));
	d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerFrame, m_cbPerFrame);

	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		Shape* shapeObject = dynamic_cast<Shape*>(*it);
		if (shapeObject)
		{
			shapeObject->UpdateConstantBufferPerObject();
		}
	}
}

void SkyGame::GenerateViewMatrixByIndex(float x, float y, float z, int i, DirectX::XMMATRIX & view)
{
	// Generate the cube map about the given position.
	XMFLOAT4 center(x, y, z, 1.f);

	// Look along each coordinate axis.
	XMFLOAT4 targets[6] =
	{
		XMFLOAT4(x + 1.0f, y, z, 1.f), // +X
		XMFLOAT4(x - 1.0f, y, z, 1.f), // -X
		XMFLOAT4(x, y + 1.0f, z, 1.f), // +Y
		XMFLOAT4(x, y - 1.0f, z, 1.f), // -Y
		XMFLOAT4(x, y, z + 1.0f, 1.f), // +Z
		XMFLOAT4(x, y, z - 1.0f, 1.f)  // -Z
	};

	// Use world up vector (0,1,0) for all directions except +Y/-Y.  In these cases, we
	// are looking down +Y or -Y, so we need a different "up" vector.
	XMFLOAT4 ups[6] =
	{
		XMFLOAT4(0.0f, 1.0f, 0.0f, 0.f),  // +X
		XMFLOAT4(0.0f, 1.0f, 0.0f, 0.f),  // -X
		XMFLOAT4(0.0f, 0.0f, -1.0f, 0.f), // +Y
		XMFLOAT4(0.0f, 0.0f, +1.0f, 0.f), // -Y
		XMFLOAT4(0.0f, 1.0f, 0.0f, 0.f),  // +Z
		XMFLOAT4(0.0f, 1.0f, 0.0f, 0.f)   // -Z
	};

	XMVECTOR pos = XMLoadFloat4(&center);
	XMVECTOR target = XMLoadFloat4(&targets[i]);
	XMVECTOR up = XMLoadFloat4(&ups[i]);

	view = XMMatrixLookAtLH(pos, target, up);
}

void SkyBox::Initialize(Microsoft::WRL::ComPtr<ID3D11Device>& device, Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, DirectX::XMFLOAT4X4 * view, DirectX::XMFLOAT4X4 * proj)
{
	Super::Initialize(device, context, view, proj);

	// Make sure the depth function is LESS_EQUAL and not just LESS.  
	// Otherwise, the normalized depth values at z = 1 (NDC) will 
	// fail the depth test if the depth buffer was cleared to 1.
	CD3D11_DEPTH_STENCIL_DESC dsDesc(D3D11_DEFAULT);
	dsDesc.DepthFunc = D3D11_COMPARISON_LESS_EQUAL;
	HRESULT hr = m_d3dDevice->CreateDepthStencilState(&dsDesc, m_depthStencilState.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void SkyBox::Render()
{
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(VertexPosition);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);
	m_d3dContext->VSSetConstantBuffers(1, 1, m_constantBufferPerObject.GetAddressOf());

	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	m_d3dContext->PSSetShaderResources(1, 1, m_diffuseMapView.GetAddressOf());

	ComPtr<ID3D11SamplerState> previousSamplerState;
	m_d3dContext->PSGetSamplers(0, 1, previousSamplerState.GetAddressOf());
	m_d3dContext->PSSetSamplers(0, 1, m_samplerState.GetAddressOf());

	ComPtr<ID3D11DepthStencilState> prevDepthStencilState;
	UINT prevStencilRef;
	m_d3dContext->OMGetDepthStencilState(prevDepthStencilState.GetAddressOf(), &prevStencilRef);
	m_d3dContext->OMSetDepthStencilState(m_depthStencilState.Get(), 0);

	m_d3dContext->DrawIndexed(m_indexCount, 0, 0);

	m_d3dContext->PSSetSamplers(0, 1, previousSamplerState.GetAddressOf());
	m_d3dContext->OMSetDepthStencilState(prevDepthStencilState.Get(), prevStencilRef);
}

void SkyBox::BuildShape()
{
	//  | height
	//  |    
	//  |   / width
	//  |  /
	//  | /
	//  |/---------- length

	// Set vertex buffer
	
	float halfLength = m_length / 2;
	float halfWidth = m_width / 2;
	float halfHeight = m_height / 2;

	VertexPosition vertices[] =
	{
		{ XMFLOAT3(-halfLength, -halfHeight, -halfWidth) },
		{ XMFLOAT3(-halfLength, +halfHeight, -halfWidth) },
		{ XMFLOAT3(+halfLength, +halfHeight, -halfWidth) },
		{ XMFLOAT3(+halfLength, -halfHeight, -halfWidth) },
		{ XMFLOAT3(-halfLength, -halfHeight, +halfWidth) },
		{ XMFLOAT3(-halfLength, +halfHeight, +halfWidth) },
		{ XMFLOAT3(+halfLength, +halfHeight, +halfWidth) },
		{ XMFLOAT3(+halfLength, -halfHeight, +halfWidth) }
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
		0, 2, 1,
		0, 3, 2,

		// back face
		4, 5, 6,
		4, 6, 7,

		// left face
		4, 1, 5,
		4, 0, 1,

		// right face
		3, 6, 2,
		3, 7, 6,

		// top face
		1, 6, 5,
		1, 2, 6,

		// bottom face
		4, 3, 0,
		4, 7, 3
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
}

void SkyBox::BuildShader()
{
	const std::wstring shaderFilename = L"SkyGame\\Sky.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename, nullptr);
}

void SkyBox::BuildConstantBuffer()
{
	CreateConstantBufferPerObject(sizeof(cbPerObjectStruct));
}

void SkyBox::BuildTexture()
{
	BuildTextureByName(L"SkyGame\\grasscube1024.dds", m_diffuseMapView);

	CD3D11_SAMPLER_DESC samplerDesc(D3D11_DEFAULT);
	samplerDesc.Filter = D3D11_FILTER_ANISOTROPIC;
	samplerDesc.MaxAnisotropy = 4;
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	HRESULT hr = m_d3dDevice->CreateSamplerState(&samplerDesc, m_samplerState.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void SkyBox::SetInputLayout()
{
	D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
	{
		{"POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT,0,0,D3D11_INPUT_PER_VERTEX_DATA,0}
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

void ReflectBox::Initialize(Microsoft::WRL::ComPtr<ID3D11Device>& device, Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, DirectX::XMFLOAT4X4 * view, DirectX::XMFLOAT4X4 * proj)
{
	Super::Initialize(device, context, view, proj);

	WorldTransform(XMMatrixTranslation(0.f, 17.f, 0.f));
}

void ReflectBox::Render()
{
	if(!bUsingDynamicCubeMap)
		RenderInternal(m_cubeMapView);
}

void ReflectBox::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(0.0f, 0.0f, 0.0f, 1.0f);
	m_cbPerObject.material.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);
	m_cbPerObject.material.reflect = XMFLOAT4(0.9f, 0.9f, 0.9f, 0.f);
}

void ReflectBox::BuildShader()
{
	const D3D_SHADER_MACRO alphaTestDefines[] =
	{
#if ENABLEFOG
		"FOG", "1",
#endif
		"ALPHA_TEST", "1",
		"SKY_REFLECTION", "1",
		NULL, NULL
	};

	const std::wstring shaderFilename = L"TransparentWaveGame\\Blending.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename, alphaTestDefines);
}

void ReflectBox::BuildTexture()
{
	BuildTextureByName(L"MirrorGame\\ice.dds", m_diffuseMapView);
	BuildTextureByName(L"SkyGame\\grasscube1024.dds", m_cubeMapView);
}

void ReflectBox::RenderInternal(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& cubeMapSRV)
{
	m_d3dContext->PSSetShaderResources(1, 1, cubeMapSRV.GetAddressOf());

	Super::Render();
}

void ReflectSphere::BuildShape()
{
	using VertexType = VertexPositionNormalUV;

	std::vector<VertexType> vertices;
	std::vector<UINT> indices;

	float radius = 1.0f * m_scale;
	UINT sliceCount = 20;
	UINT stackCount = 20;

	//
	// Compute the vertices stating at the top pole and moving down the stacks.
	//

	// Poles: note that there will be texture coordinate distortion as there is
	// not a unique point on the texture map to assign to the pole when mapping
	// a rectangular texture onto a sphere.
	VertexType topVertex = { XMFLOAT3(0.0f, +radius, 0.0f), XMFLOAT3(0.0f, +1.0f, 0.0f), XMFLOAT2(0.0f, 0.0f) };
	VertexType bottomVertex = { XMFLOAT3(0.0f, -radius, 0.0f), XMFLOAT3(0.0f, -1.0f, 0.0f), XMFLOAT2(0.0f, 1.0f) };

	vertices.push_back(topVertex);

	float phiStep = XM_PI / stackCount;
	float thetaStep = 2.0f*XM_PI / sliceCount;

	// Compute vertices for each stack ring (do not count the poles as rings).
	for (UINT i = 1; i <= stackCount - 1; ++i)
	{
		float phi = i * phiStep;

		// Vertices of ring.
		for (UINT j = 0; j <= sliceCount; ++j)
		{
			float theta = j * thetaStep;

			VertexType v;

			// spherical to cartesian
			v.position.x = radius * sinf(phi)*cosf(theta);
			v.position.y = radius * cosf(phi);
			v.position.z = radius * sinf(phi)*sinf(theta);

			XMVECTOR p = XMLoadFloat3(&v.position);
			XMStoreFloat3(&v.normal, XMVector3Normalize(p));

			v.textureUV.x = theta / XM_2PI;
			v.textureUV.y = phi / XM_PI;

			vertices.push_back(v);
		}
	}

	vertices.push_back(bottomVertex);

	//
	// Compute indices for top stack.  The top stack was written first to the vertex buffer
	// and connects the top pole to the first ring.
	//

	for (UINT i = 1; i <= sliceCount; ++i)
	{
		indices.push_back(0);
		indices.push_back(i + 1);
		indices.push_back(i);
	}

	//
	// Compute indices for inner stacks (not connected to poles).
	//

	// Offset the indices to the index of the first vertex in the first ring.
	// This is just skipping the top pole vertex.
	UINT baseIndex = 1;
	UINT ringVertexCount = sliceCount + 1;
	for (UINT i = 0; i < stackCount - 2; ++i)
	{
		for (UINT j = 0; j < sliceCount; ++j)
		{
			indices.push_back(baseIndex + i * ringVertexCount + j);
			indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices.push_back(baseIndex + (i + 1)*ringVertexCount + j);

			indices.push_back(baseIndex + (i + 1)*ringVertexCount + j);
			indices.push_back(baseIndex + i * ringVertexCount + j + 1);
			indices.push_back(baseIndex + (i + 1)*ringVertexCount + j + 1);
		}
	}

	//
	// Compute indices for bottom stack.  The bottom stack was written last to the vertex buffer
	// and connects the bottom pole to the bottom ring.
	//

	// South pole vertex was added last.
	UINT southPoleIndex = (UINT)vertices.size() - 1;

	// Offset the indices to the index of the first vertex in the last ring.
	baseIndex = southPoleIndex - ringVertexCount;

	for (UINT i = 0; i < sliceCount; ++i)
	{
		indices.push_back(southPoleIndex);
		indices.push_back(baseIndex + i);
		indices.push_back(baseIndex + i + 1);
	}

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = sizeof(VertexType) * vertices.size();
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vbInitData;
	vbInitData.pSysMem = vertices.data();
	vbInitData.SysMemPitch = 0;
	vbInitData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_indexCount = indices.size();

	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = sizeof(UINT) * indices.size();
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA ibInitData;
	ibInitData.pSysMem = indices.data();
	ibInitData.SysMemPitch = 0;
	ibInitData.SysMemSlicePitch = 0;

	hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibInitData, m_indexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void SkySphere::BuildShape()
{
	using VertexType = VertexPositionNormalUV;

	GeometryGenerator geo;
	GeometryGenerator::MeshData meshData;
	float radius = sqrt(m_length*m_length + m_height * m_height + m_width * m_width);
	geo.CreateSphere(radius, 20, 20, meshData);

	std::vector<VertexType> vertices(meshData.Vertices.size());

	for (int i = 0; i < meshData.Vertices.size(); ++i)
	{
		vertices[i].position = meshData.Vertices[i].Position;
		vertices[i].normal = meshData.Vertices[i].Normal;
		vertices[i].textureUV = meshData.Vertices[i].TexC;
	}

	std::vector<UINT> indices(meshData.Indices);

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = sizeof(VertexType) * vertices.size();
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vbInitData;
	vbInitData.pSysMem = vertices.data();
	vbInitData.SysMemPitch = 0;
	vbInitData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_indexCount = indices.size();

	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = sizeof(UINT) * indices.size();
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA ibInitData;
	ibInitData.pSysMem = indices.data();
	ibInitData.SysMemPitch = 0;
	ibInitData.SysMemSlicePitch = 0;

	hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibInitData, m_indexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
}