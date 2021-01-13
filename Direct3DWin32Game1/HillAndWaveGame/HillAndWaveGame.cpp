#include "pch.h"
#include "HillAndWaveGame\HillAndWaveGame.h"
#include "Common/VertexStructuer.h"
#include <time.h>

using namespace DirectX;
using Microsoft::WRL::ComPtr;
using VertexType = VertexPositionNormalColor;

HillAndWaveGame::~HillAndWaveGame()
{
	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		delete (*it);
	}
}

void HillAndWaveGame::Initialize(HWND window, int width, int height)
{
	m_objects.push_back(new Hill());
	m_objects.push_back(new Wave());

	m_initCameraY = 150.f;
	m_initCameraZ = -150.f;
	m_maxRadius = 300.f;
	m_mouseMoveRate = 15.f;

	Super::Initialize(window, width, height);
}

inline float Hill::GetHeight(float x, float z) const
{
	return 0.3f*(z*sinf(0.1f*x) + x * cosf(0.1f*z));
}

inline XMFLOAT3 Hill::GetHillNormal(float x, float z) const
{
	// n = (-df/dx, 1, -df/dz)
	XMFLOAT3 n(
		-0.03f*z*cosf(0.1f*x) - 0.3f*cosf(0.1f*z),
		1.0f,
		-0.3f*sinf(0.1f*x) + 0.03f*x*sinf(0.1f*z));

	XMVECTOR unitNormal = XMVector3Normalize(XMLoadFloat3(&n));
	XMStoreFloat3(&n, unitNormal);

	return n;
}

void Hill::BuildShape()
{
	// CreateGrid

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

	VertexType* vertices = new VertexType[vertexCount];

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

			vertices[i*n + j].normal = GetHillNormal(x, z);
		}
	}

	// Vertex buffer

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = sizeof(VertexType) * vertexCount;;
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

	//
	// Create the indices.
	//

	UINT* indices = new UINT[faceCount * 3];

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

	// Index buffer

	m_indexCount = faceCount * 3;

	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = sizeof(UINT) * m_indexCount;;
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

	delete[] vertices;
	delete[] indices;
}

Wave::Wave()
{
	float dt = m_timeStep;
	float dx = m_spatialStep;

	float d = m_damping * dt + 2.0f;
	float e = (m_speed*m_speed)*(dt*dt) / (dx*dx);
	mK1 = (m_damping*dt - 2.0f) / d;
	mK2 = (4.0f - 8.0f*e) / d;
	mK3 = (2.0f*e) / d;

	m_prevSolution = new XMFLOAT3[m_numRows*m_numCols];
	m_currSolution = new XMFLOAT3[m_numRows*m_numCols];

	// Generate grid vertices in system memory.
	UINT m = m_numRows;
	UINT n = m_numCols;

	float halfWidth = (n - 1)*dx*0.5f;
	float halfDepth = (m - 1)*dx*0.5f;

	for (UINT i = 0; i < m; ++i)
	{
		float z = halfDepth - i * dx;
		for (UINT j = 0; j < n; ++j)
		{
			float x = -halfWidth + j * dx;

			m_prevSolution[i*n + j] = XMFLOAT3(x, 0.0f, z);
			m_currSolution[i*n + j] = XMFLOAT3(x, 0.0f, z);
		}
	}
}

Wave::~Wave()
{
	delete[] m_prevSolution;
	delete[] m_currSolution;
}

void Wave::BuildShape()
{
	// Create the vertex buffer.  Note that we allocate space only, as
	// we will be updating the data every time step of the simulation.

	const UINT vertexCount = m_numRows * m_numCols;

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = sizeof(VertexType) * vertexCount;
	vbDesc.Usage = D3D11_USAGE_DYNAMIC;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, nullptr, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Test
	bool bTest = false;
	if (bTest)
	{
		VertexType* vertices = new VertexType[vertexCount];

		for (UINT i = 0; i < vertexCount; ++i)
		{
			vertices[i].position = m_currSolution[i];
			vertices[i].color = XMFLOAT4(Colors::Red);
			vertices[i].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
		}

		D3D11_SUBRESOURCE_DATA vbInitData;
		vbInitData.pSysMem = vertices;
		vbInitData.SysMemPitch = 0;
		vbInitData.SysMemSlicePitch = 0;

		HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
		DX::ThrowIfFailed(hr);

		delete[] vertices;
	}

	// Create the index buffer.  The index buffer is fixed, so we only 
	// need to create and set once.

	const UINT triangleCount = (m_numRows - 1)*(m_numCols - 1) * 2;
	//UINT indices[triangleCount * 3];
	UINT* indices = new UINT[triangleCount * 3];

	// Iterate over each quad.
	UINT m = m_numRows;
	UINT n = m_numCols;
	int k = 0;
	for (UINT i = 0; i < m - 1; ++i)
	{
		for (DWORD j = 0; j < n - 1; ++j)
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

	m_indexCount = triangleCount * 3;

	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = sizeof(UINT) * m_indexCount;
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

	delete[] indices;

	time_t t;
	srand((unsigned) time(&t));
}

void Wave::Update(DX::StepTimer const & timer)
{
	//
	// Every quarter second, generate a random wave.
	//
	static float t_base = 0.0f;
	if ((timer.GetTotalSeconds() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		DWORD i = 5 + rand() % (m_numRows - 10);
		DWORD j = 5 + rand() % (m_numCols - 10);

		// Don't disturb boundaries.
		assert(i > 1 && i < m_numRows - 2);
		assert(j > 1 && j < m_numCols - 2);

		float magnitude = 1.0f + (float)(rand()) / (float)RAND_MAX;

		float halfMag = 0.5f*magnitude;

		// Disturb the ijth vertex height and its neighbors.
		m_currSolution[i*m_numCols + j].y += magnitude;
		m_currSolution[i*m_numCols + j + 1].y += halfMag;
		m_currSolution[i*m_numCols + j - 1].y += halfMag;
		m_currSolution[(i + 1)*m_numCols + j].y += halfMag;
		m_currSolution[(i - 1)*m_numCols + j].y += halfMag;
	}

	//
	// Update waves
	//

	static float t = 0;

	// Accumulate time.
	float dt = float(timer.GetElapsedSeconds());
	t += dt;

	// Only update the simulation at the specified time step.
	if (t >= m_timeStep)
	{
		// Only update interior points; we use zero boundary conditions.
		for (DWORD i = 1; i < m_numRows - 1; ++i)
		{
			for (DWORD j = 1; j < m_numCols - 1; ++j)
			{
				// After this update we will be discarding the old previous
				// buffer, so overwrite that buffer with the new update.
				// Note how we can do this inplace (read/write to same element) 
				// because we won't need prev_ij again and the assignment happens last.

				// Note j indexes x and i indexes z: h(x_j, z_i, t_k)
				// Moreover, our +z axis goes "down"; this is just to 
				// keep consistent with our row indices going down.

				m_prevSolution[i*m_numCols + j].y =
					mK1 * m_prevSolution[i*m_numCols + j].y +
					mK2 * m_currSolution[i*m_numCols + j].y +
					mK3 * (m_currSolution[(i + 1)*m_numCols + j].y +
						m_currSolution[(i - 1)*m_numCols + j].y +
						m_currSolution[i*m_numCols + j + 1].y +
						m_currSolution[i*m_numCols + j - 1].y);
			}
		}

		// We just overwrote the previous buffer with the new data, so
		// this data needs to become the current solution and the old
		// current solution becomes the new previous solution.
		std::swap(m_prevSolution, m_currSolution);

		t = 0.0f; // reset time
	}

	//
	// Update the wave vertex buffer with the new solution.
	//

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HRESULT hr = m_d3dContext->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

	VertexType* v = reinterpret_cast<VertexType*>(mappedData.pData);
	const UINT vertexCount = m_numRows * m_numCols;
	for (UINT i = 0; i < vertexCount; ++i)
	{
		v[i].position = m_currSolution[i];
		v[i].color = XMFLOAT4(Colors::Blue);
		v[i].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
	}

	m_d3dContext->Unmap(m_vertexBuffer.Get(), 0);

	Super::Update(timer);
}