#include "pch.h"
#include "InstancingGame/InstancingGame.h"
#include "DDSTextureLoader.h"
#include "DirectXCollision.h"
#include <sstream>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct MaterialData
{
	Material Mat;
	UINT     DiffuseMapIndex;
	UINT     MatPad0;
	UINT     MatPad1;
	UINT     MatPad2;
};

void InstancingGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	BuildInstancedBuffer();
}

void InstancingGame::OnKeyButtonReleased(WPARAM key)
{
	Super::OnKeyButtonReleased(key);

	if (key == 'C')
	{
		ToggleFrustumCulling();
	}
}

void InstancingGame::ToggleFrustumCulling()
{
	bFrustumCullingEnable = !bFrustumCullingEnable;
}

void InstancingGame::OnMouseDown(WPARAM btnState, int x, int y)
{
	Super::OnMouseDown(btnState, x, y);

	if (bUsingOrbitCamera && ((btnState & MK_RBUTTON) != 0))
	{
		Pick(x, y);
	}
	else if (!bUsingOrbitCamera && ((btnState & MK_LBUTTON) != 0))
	{
		Pick(x, y);
	}
}

void InstancingGame::Update(DX::StepTimer const & timer)
{
	Super::Update(timer);
}

void InstancingGame::AddObjects()
{
	m_instanceCrate = new InstancingCrate();
	m_objects.push_back(m_instanceCrate);

	m_pickedTriangle = new PickedTriangle();
	m_objects.push_back(m_pickedTriangle);
}

void InstancingGame::PostObjectsRender()
{
	// Frustum Culling

	if (bFrustumCullingEnable)
	{
		m_cullingDataArray.clear();

		BoundingFrustum frustum;
		XMMATRIX proj = XMLoadFloat4x4(&m_proj);
		BoundingFrustum::CreateFromMatrix(frustum, proj);

		XMMATRIX view = XMLoadFloat4x4(&m_view);
		XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

		for (int i = 0; i < m_instancedDataArray.size(); ++i)
		{
			BoundingFrustum localFrustum;

			XMMATRIX world = XMMatrixTranspose(XMLoadFloat4x4(&m_instancedDataArray[i].World));
			XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);
			XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

			frustum.Transform(localFrustum, viewToLocal);

			if (localFrustum.Contains(*(m_instanceCrate->m_bounds)) != ContainmentType::DISJOINT)
			{
				m_cullingDataArray.push_back(m_instancedDataArray[i]);
			}
		}

		m_d3dContext->UpdateSubresource(m_instanceDataBuffer.Get(), 0, nullptr, m_cullingDataArray.data(), 0, 0);

		m_d3dContext->VSSetShaderResources(1, 1, m_instanceDataSRV.GetAddressOf());
		m_d3dContext->VSSetConstantBuffers(0, 1, m_constantBufferPerFrame.GetAddressOf());

		m_d3dContext->DrawIndexedInstanced(m_instanceCrate->m_indexCount, m_cullingDataArray.size(), 0, 0, 0);
	}
	else
	{
		m_d3dContext->UpdateSubresource(m_instanceDataBuffer.Get(), 0, nullptr, m_instancedDataArray.data(), 0, 0);

		m_d3dContext->DrawIndexedInstanced(m_instanceCrate->m_indexCount,m_instanceCount, 0, 0, 0);
	}

	// Render picked triangle

	if (bNewPicked && bPickSuccess)
	{
		UINT i0 = m_instanceCrate->m_indices[m_pickedTriangleIndex];
		UINT i1 = m_instanceCrate->m_indices[m_pickedTriangleIndex + 1];
		UINT i2 = m_instanceCrate->m_indices[m_pickedTriangleIndex + 2];

		XMVECTOR v0Vec = XMLoadFloat3(&m_instanceCrate->m_vertices[i0].position);
		XMVECTOR v1Vec = XMLoadFloat3(&m_instanceCrate->m_vertices[i1].position);
		XMVECTOR v2Vec = XMLoadFloat3(&m_instanceCrate->m_vertices[i2].position);

		XMFLOAT3 n0 = m_instanceCrate->m_vertices[i0].normal;
		XMFLOAT3 n1 = m_instanceCrate->m_vertices[i1].normal;
		XMFLOAT3 n2 = m_instanceCrate->m_vertices[i2].normal;

		// Move triangle a lit bit higher than before to avoid z fighting
		XMVECTOR triangleNormal = XMLoadFloat3(&n0); // Since all normal of three vectors is the same, we just take one as the plane normal.
		XMVECTOR offset = triangleNormal * 0.01;
		XMMATRIX translation = XMMatrixTranslationFromVector(offset);

		v0Vec = XMVector3TransformCoord(v0Vec, translation);
		v1Vec = XMVector3TransformCoord(v1Vec, translation);
		v2Vec = XMVector3TransformCoord(v2Vec, translation);

		XMFLOAT3 v0, v1, v2;
		XMStoreFloat3(&v0, v0Vec);
		XMStoreFloat3(&v1, v1Vec);
		XMStoreFloat3(&v2, v2Vec);

		VertexPositionNormalColor vertices[3] =
		{
			{ v0, n0, XMFLOAT4(Colors::Yellow) },
			{ v1, n1, XMFLOAT4(Colors::Yellow) },
			{ v2, n2, XMFLOAT4(Colors::Yellow) }
		};

		m_d3dContext->UpdateSubresource(m_pickedTriangle->m_vertexBuffer.Get(), 0, nullptr, vertices, 0, 0);

		bNewPicked = false;
	}

	if (bPickSuccess)
	{
		XMMATRIX world = XMMatrixTranspose(XMLoadFloat4x4(&m_instancedDataArray[m_pickedInstaceIndex].World));
		XMMATRIX view = XMLoadFloat4x4(&m_view);
		XMMATRIX proj = XMLoadFloat4x4(&m_proj);

		XMMATRIX worldViewProjTranspose = XMMatrixTranspose(XMMatrixMultiply(XMMatrixMultiply(world, view), proj));

		d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_pickedTriangle->m_constantBufferPerObject, worldViewProjTranspose);

		m_d3dContext->IASetInputLayout(m_pickedTriangle->m_inputLayout.Get());
		UINT stride = sizeof(VertexPositionNormalColor);
		UINT offset = 0;
		m_d3dContext->IASetVertexBuffers(0, 1, m_pickedTriangle->m_vertexBuffer.GetAddressOf(), &stride, &offset);
		m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		m_d3dContext->VSSetConstantBuffers(0, 1, m_pickedTriangle->m_constantBufferPerObject.GetAddressOf());
		m_d3dContext->VSSetShader(m_pickedTriangle->m_vertexShader.Get(), nullptr, 0);
		m_d3dContext->PSSetShader(m_pickedTriangle->m_pixelShader.Get(), nullptr, 0);

		m_d3dContext->Draw(3, 0);
	}
}

void InstancingGame::BuildConstantBuffer()
{	
	CreateConstantBufferPerFrame(sizeof(cbPerFrame));
}

void InstancingGame::UpdateConstantBufferPerFrame()
{
	m_cbPerFrame.dirLight = m_dirLight;
	m_cbPerFrame.pointLight = m_pointLight;
	m_cbPerFrame.spotLight = m_spotLight;
	m_cbPerFrame.eyePosW = m_eyePos;
	m_cbPerFrame.fogColor = XMFLOAT4(Colors::Silver);
	m_cbPerFrame.fogStart = 15.f;
	m_cbPerFrame.fogRange = 175.f;

	XMMATRIX view = XMLoadFloat4x4(&m_view);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMStoreFloat4x4(&m_cbPerFrame.viewProj, XMMatrixTranspose(viewProj));

	d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerFrame, m_cbPerFrame);
}

void InstancingGame::CalculateFrameStats()
{
	// Code computes the average frames per second, and also the 
// average time it takes to render one frame.  These stats 
// are appended to the window caption bar.

	static int frameCnt = 0;
	static float timeElapsed = 0.0f;

	frameCnt++;

	// Compute averages over one second period.
	if ((m_timer.GetTotalSeconds() - timeElapsed) >= 1.0f)
	{
		float fps = (float)frameCnt; // fps = frameCnt / 1
		float mspf = 1000.0f / fps;

		std::wostringstream outs;
		outs.precision(6);

		outs << "DirectX3DWin32Game" << L"    "
			<< L"FPS: " << fps << L"    "
			<< L"Frame Time: " << mspf << L" (ms)";

		outs << L"    " <<
			(bFrustumCullingEnable ? m_cullingDataArray.size() : m_instanceCount) <<
			L" objects visible out of " << m_instanceCount;

		SetWindowText(m_window, outs.str().c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void InstancingGame::BuildInstancedBuffer()
{
	float width = 200.0f;
	float height = 200.0f;
	float depth = 200.0f;

	int instanceN = 5;

	m_instanceCount = instanceN * instanceN*instanceN;
	
	m_instancedDataArray.resize(m_instanceCount);
	m_cullingDataArray.reserve(m_instanceCount);

	float x = -0.5f*width;
	float y = -0.5f*height;
	float z = -0.5f*depth;
	float dx = width / (instanceN - 1);
	float dy = height / (instanceN - 1);
	float dz = depth / (instanceN - 1);
	for (int k = 0; k < instanceN; ++k)
	{
		for (int i = 0; i < instanceN; ++i)
		{
			for (int j = 0; j < instanceN; ++j)
			{
				// Position instanced along a 3D grid.

				//instancedDataArray[k*INSTANCE_N*INSTANCE_N + i * INSTANCE_N + j].World = XMFLOAT4X4(
				//	1.0f, 0.0f, 0.0f, 0.0f,
				//	0.0f, 1.0f, 0.0f, 0.0f,
				//	0.0f, 0.0f, 1.0f, 0.0f,
				//	x + j * dx, y + i * dy, z + k * dz, 1.0f);

				// We need transpose here.
				m_instancedDataArray[k*instanceN*instanceN + i * instanceN + j].World = XMFLOAT4X4(
					1.0f, 0.0f, 0.0f, x + j * dx,
					0.0f, 1.0f, 0.0f, y + i * dy,
					0.0f, 0.0f, 1.0f, z + k * dz,
					0.0f, 0.0f, 0.0f, 1.0f);

				m_instancedDataArray[k*instanceN*instanceN + i * instanceN + j].MaterialIndex = rand() % m_instanceCrate->m_matCount;
			}
		}
	}

	UINT byteWidth = m_instanceCount * sizeof(InstanceData);
	CD3D11_BUFFER_DESC instanceDataDesc(byteWidth, D3D11_BIND_SHADER_RESOURCE);
	instanceDataDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	instanceDataDesc.StructureByteStride = sizeof(InstanceData);

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = m_instancedDataArray.data();
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&instanceDataDesc, &initData, m_instanceDataBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_BUFFER);
	srvDesc.Buffer.NumElements = m_instanceCount;

	hr = m_d3dDevice->CreateShaderResourceView(m_instanceDataBuffer.Get(), &srvDesc, m_instanceDataSRV.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void InstancingGame::Pick(int x, int y)
{
	bNewPicked = true;

	XMFLOAT4X4 proj = m_proj;

	float vx = (+2.0f*x / m_outputWidth - 1.0f) / proj(0, 0);
	float vy = (-2.0f*y / m_outputHeight + 1.0f) / proj(1, 1);

	XMVECTOR origin = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	XMVECTOR dir = XMVectorSet(vx, vy, 1.f, 0.f);

	XMMATRIX view = XMLoadFloat4x4(&m_view);
	XMMATRIX invView = XMMatrixInverse(&XMMatrixDeterminant(view), view);

	float tmin = FLT_MAX;
	bPickSuccess = false;
	for (int i = 0; i < m_instancedDataArray.size(); ++i)
	{
		XMMATRIX world = XMMatrixTranspose(XMLoadFloat4x4(&m_instancedDataArray[i].World));
		XMMATRIX invWorld = XMMatrixInverse(&XMMatrixDeterminant(world), world);

		XMMATRIX viewToLocal = XMMatrixMultiply(invView, invWorld);

		XMVECTOR localOrigin = XMVector3TransformCoord(origin, viewToLocal);
		XMVECTOR localDir = XMVector3TransformNormal(dir, viewToLocal);
		localDir = XMVector3Normalize(localDir);

		float tb = 0.f;
		if (m_instanceCrate->m_bounds->Intersects(localOrigin, localDir, tb))
		{
			for (int j = 0; j < m_instanceCrate->m_indices.size(); j += 3)
			{
				UINT i0 = m_instanceCrate->m_indices[j];
				UINT i1 = m_instanceCrate->m_indices[j+1];
				UINT i2 = m_instanceCrate->m_indices[j+2];

				XMVECTOR v0 = XMLoadFloat3(&m_instanceCrate->m_vertices[i0].position);
				XMVECTOR v1 = XMLoadFloat3(&m_instanceCrate->m_vertices[i1].position);
				XMVECTOR v2 = XMLoadFloat3(&m_instanceCrate->m_vertices[i2].position);

				float t = FLT_MAX;
				if (TriangleTests::Intersects(localOrigin, localDir, v0, v1, v2, t))
				{
					if (t < tmin)
					{
						bPickSuccess = true;
						tmin = t;
						m_pickedInstaceIndex = i;
						m_pickedTriangleIndex = j;
					}
				}
			}
		}
	}
}

InstancingCrate::InstancingCrate()
{
	m_bounds = new BoundingBox();
}

InstancingCrate::~InstancingCrate()
{
	delete m_bounds;
}

void InstancingCrate::Initialize(
	Microsoft::WRL::ComPtr<ID3D11Device>& device, 
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context, 
	DirectX::XMFLOAT4X4 * view, 
	DirectX::XMFLOAT4X4 * proj)
{
	Shape::Initialize(device, context, view, proj);

	// We need to know how many textures first
	BuildTexture();
	BuildMaterial();
}

void InstancingCrate::Update(DX::StepTimer const & timer)
{
	// We removed per object constant buffer, so do nothing here.
}

void InstancingCrate::Render()
{
	m_d3dContext->IASetInputLayout(m_inputLayout.Get());
	UINT stride = sizeof(VertexType);
	UINT offset = 0;
	m_d3dContext->IASetVertexBuffers(0, 1, m_vertexBuffer.GetAddressOf(), &stride, &offset);
	m_d3dContext->IASetIndexBuffer(m_indexBuffer.Get(), DXGI_FORMAT_R32_UINT, 0);
	m_d3dContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

	m_d3dContext->VSSetShader(m_vertexShader.Get(), nullptr, 0);

	m_d3dContext->PSSetShader(m_pixelShader.Get(), nullptr, 0);
	m_d3dContext->PSSetShaderResources(0, 1, m_diffuseMapView.GetAddressOf());
	m_d3dContext->PSSetShaderResources(2, 1, m_matDataSRV.GetAddressOf());

	// Move draw call to InstancingGame
}

void InstancingCrate::BuildTexture()
{
	std::vector<std::wstring> treeFilenames;
	treeFilenames.push_back(L"MirrorGame/brick01.dds");
	treeFilenames.push_back(L"MirrorGame/checkboard.dds");
	treeFilenames.push_back(L"MirrorGame/ice.dds");
	treeFilenames.push_back(L"InstancingGame/bricks.dds");

	m_texArraySize = treeFilenames.size();

	std::vector<ComPtr<ID3D11Texture2D>> srcTextures(m_texArraySize);
	std::vector<ComPtr<ID3D11Resource>> textureResources(m_texArraySize);

	for (int i = 0; i < m_texArraySize; ++i)
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
	texArrayDesc.ArraySize = m_texArraySize;
	texArrayDesc.Usage = D3D11_USAGE_DEFAULT;
	texArrayDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	texArrayDesc.CPUAccessFlags = 0;

	ComPtr<ID3D11Texture2D> texArray;
	HRESULT hr = m_d3dDevice->CreateTexture2D(&texArrayDesc, nullptr, texArray.GetAddressOf());
	DX::ThrowIfFailed(hr);

	for (int i = 0; i < m_texArraySize; ++i)
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
	texArraySRVDesc.Texture2DArray.ArraySize = m_texArraySize;

	hr = m_d3dDevice->CreateShaderResourceView(texArray.Get(), &texArraySRVDesc, m_diffuseMapView.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void InstancingCrate::BuildShader()
{
	const std::wstring shaderFilename = L"InstancingGame\\InstancingObjectWithUV.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename, nullptr);
}

void InstancingCrate::BuildShape()
{
	// Set vertex buffer
	VertexType vertices[4*6] =
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

	XMFLOAT3 vMinF3(FLT_MAX, FLT_MAX, FLT_MAX);
	XMFLOAT3 vMaxF3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
	XMVECTOR vMin = XMLoadFloat3(&vMinF3);
	XMVECTOR vMax = XMLoadFloat3(&vMaxF3);

	int vertexCount = ARRAYSIZE(vertices);
	for (int i = 0; i < vertexCount; ++i)
	{
		vertices[i].position.x *= scale;
		vertices[i].position.y *= scale;
		vertices[i].position.z *= scale;

		vertices[i].position.y += positionOffset;

		m_vertices.push_back(vertices[i]);

		XMVECTOR pos = XMLoadFloat3(&vertices[i].position);
		vMin = XMVectorMin(pos, vMin);
		vMax = XMVectorMax(pos, vMax);
	}

	XMStoreFloat3(&(m_bounds->Center), 0.5f*(vMin + vMax));
	XMStoreFloat3(&(m_bounds->Extents), 0.5f*(vMax - vMin));

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
	UINT indices[6*6] =
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

	for (int i = 0; i < m_indexCount; ++i)
	{
		m_indices.push_back(indices[i]);
	}

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

void InstancingCrate::BuildMaterial()
{
	Material mat;
	mat.ambient = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	mat.diffuse = XMFLOAT4(1.0f, 1.0f, 1.0f, 1.0f);
	mat.specular = XMFLOAT4(0.4f, 0.4f, 0.4f, 16.0f);

	std::vector<MaterialData> matDataArray;
	for (int i = 0; i < m_matCount; ++i)
	{
		MaterialData matData;
		matData.Mat = mat;
		matData.DiffuseMapIndex = i % m_texArraySize;
		matDataArray.push_back(matData);
	}

	UINT byteWidth = m_matCount * sizeof(MaterialData);
	CD3D11_BUFFER_DESC matDataDesc(byteWidth, D3D11_BIND_SHADER_RESOURCE);
	matDataDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	matDataDesc.StructureByteStride = sizeof(MaterialData);

	D3D11_SUBRESOURCE_DATA initData;
	initData.pSysMem = matDataArray.data();
	initData.SysMemPitch = 0;
	initData.SysMemSlicePitch = 0;

	ComPtr<ID3D11Buffer> m_matDataBuffer;
	HRESULT hr = m_d3dDevice->CreateBuffer(&matDataDesc, &initData, m_matDataBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_BUFFER);
	srvDesc.Buffer.NumElements = m_matCount;

	hr = m_d3dDevice->CreateShaderResourceView(m_matDataBuffer.Get(), &srvDesc, m_matDataSRV.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void InstancingCrate::BuildConstantBuffer()
{
	// We removed per object constant buffer, so do nothing here.
}

void PickedTriangle::Render()
{
}

void PickedTriangle::BuildShape()
{
	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = sizeof(VertexPositionNormalColor) * 3;
	vbDesc.Usage = D3D11_USAGE_DEFAULT;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, nullptr, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void PickedTriangle::BuildConstantBuffer()
{
	CreateConstantBufferPerObject(sizeof(XMFLOAT4X4));
}
