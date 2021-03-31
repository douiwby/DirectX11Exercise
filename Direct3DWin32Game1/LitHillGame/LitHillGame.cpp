#include "pch.h"
#include "LitHillGame/LitHillGame.h"
#include <ctime>

using namespace DirectX;
using Microsoft::WRL::ComPtr;
#ifdef USE_VERTEX_COLOR
using VertexType = VertexPositionNormalColor; // The color is not using here
#else
using VertexType = VertexPositionNormalUV; // The UV is not using here
#endif

LitHillGame::~LitHillGame()
{
	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		if (*it)
		{
			delete (*it);
			*it = nullptr;
		}
	}
	m_objects.clear();
}

void LitHillGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	BuildLight();
	BuildConstantBuffer();

	m_d3dContext->PSSetConstantBuffers(0, 1, m_constantBufferPerFrame.GetAddressOf());
}

void LitHillGame::BuildLight()
{
	// Directional light.
	m_dirLight.Ambient = XMFLOAT4(0.2f, 0.2f, 0.2f, 1.0f);
	m_dirLight.Diffuse = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLight.Specular = XMFLOAT4(0.5f, 0.5f, 0.5f, 1.0f);
	m_dirLight.Direction = XMFLOAT3(0.57735f, -0.57735f, 0.57735f);

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
	m_spotLight.Position = XMFLOAT3(0.0f, 100.0f, 0.0f);
	m_spotLight.Direction = XMFLOAT3(0.0f, -1.0f, 0.0f);

	m_cbPerFrame.dirLight = m_dirLight;
	m_cbPerFrame.pointLight = m_pointLight;
	m_cbPerFrame.spotLight = m_spotLight;
}

void LitHillGame::BuildConstantBuffer()
{
	CreateConstantBufferPerFrame(sizeof(cbPerFrame));
}

void LitHillGame::Update(DX::StepTimer const & timer)
{
	Super::Update(timer);

	UpdateLightPosition(timer);
	UpdateConstantBufferPerFrame();
}

void LitHillGame::AddObjects()
{
	m_objects.push_back(new LitHill());
	m_objects.push_back(new LitWave());
}

void LitHillGame::UpdateLightPosition(DX::StepTimer const & timer)
{
	float elapsedTime = float(timer.GetElapsedSeconds());
	float totalTime = timer.GetTotalSeconds();

	// Circle light over the land surface.
	m_pointLight.Position.x = 70.0f*cosf(0.2f*totalTime);
	m_pointLight.Position.z = 70.0f*sinf(0.2f*totalTime);
	float x = m_pointLight.Position.x;
	float z = m_pointLight.Position.z;
	m_pointLight.Position.y = fmaxf(0.3f*(z*sinf(0.1f*x) + x * cosf(0.1f*z)), -3.0f) + 10.0f;


	// Control the spot light by keyboard.
	// Key map
	// https://docs.microsoft.com/en-us/windows/win32/inputdev/virtual-key-codes
	if (! ( (GetAsyncKeyState(MK_RBUTTON) & 0x8000) || (GetAsyncKeyState(MK_LBUTTON) & 0x8000) ) )
	{
		// Use keyboard to control the position
		float movingSpeed = 40.0f;
		if (GetAsyncKeyState(VK_LEFT) & 0x8000)
		{
			m_spotLight.Position.x -= movingSpeed * elapsedTime;
		}
		if (GetAsyncKeyState(VK_RIGHT) & 0x8000)
		{
			m_spotLight.Position.x += movingSpeed * elapsedTime;
		}
		if (GetAsyncKeyState(VK_UP) & 0x8000)
		{
			m_spotLight.Position.z += movingSpeed * elapsedTime;
		}
		if (GetAsyncKeyState(VK_DOWN) & 0x8000)
		{
			m_spotLight.Position.z -= movingSpeed * elapsedTime;
		}
		if (GetAsyncKeyState('E') & 0x8000)
		{
			m_spotLight.Position.y += movingSpeed * elapsedTime;
		}
		if (GetAsyncKeyState('Q') & 0x8000)
		{
			m_spotLight.Position.y -= movingSpeed * elapsedTime;
		}

		float rotateSpeed = 45.f / 180.f * XM_PI;
		if (GetAsyncKeyState('A') & 0x8000)  // A key
		{
			float theta = -rotateSpeed * elapsedTime;
			RotateVectorByZAxis(m_spotLight.Direction, theta);
		}
		if (GetAsyncKeyState('D') & 0x8000)  // D key
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
}

inline void LitHillGame::RotateVectorByZAxis(DirectX::XMFLOAT3& vector, float rotateRadian)
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

inline void LitHillGame::RotateVectorByXAxis(DirectX::XMFLOAT3 & vector, float rotateRadian)
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

inline void LitHillGame::RotateVector(DirectX::XMFLOAT3& vector, DirectX::XMFLOAT4X4& rotation)
{
	XMVECTOR dir = XMLoadFloat3(&vector);
	dir = XMVector3Transform(dir, XMLoadFloat4x4(&rotation));
	XMStoreFloat3(&vector, dir);
}

void LitHillGame::CreateConstantBufferPerFrame(UINT bufferSize)
{
	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = bufferSize;
	cbDesc.Usage = D3D11_USAGE_DYNAMIC;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBufferPerFrame.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void LitHillGame::UpdateConstantBufferPerFrame()
{
	m_cbPerFrame.dirLight = m_dirLight;
	m_cbPerFrame.pointLight = m_pointLight;
	m_cbPerFrame.spotLight = m_spotLight;
	m_cbPerFrame.eyePosW = m_eyePos;

	d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerFrame, m_cbPerFrame);
}

void LitHill::BuildShape()
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

#ifdef USE_VERTEX_COLOR
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
			
#else
			vertices[i*n + j].textureUV.x = j * du;
			vertices[i*n + j].textureUV.y = i * dv;
#endif

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
	cbDesc.ByteWidth = sizeof(cbPerObjectStruct);
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

void LitHill::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(0.48f, 0.77f, 0.46f, 1.0f);
	m_cbPerObject.material.specular = XMFLOAT4(0.2f, 0.2f, 0.2f, 16.0f);
	m_cbPerObject.material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
}

inline float LitHill::GetHeight(float x, float z) const
{
	return 0.3f*(z*sinf(0.1f*x) + x * cosf(0.1f*z));
}

inline DirectX::XMFLOAT3 LitHill::GetHillNormal(float x, float z) const
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

LitWave::LitWave()
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
	m_normals = new XMFLOAT3[m_numRows*m_numCols];
	m_tangentX = new XMFLOAT3[m_numRows*m_numCols];

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
			m_normals[i*n + j] = XMFLOAT3(0.0f, 1.0f, 0.0f);
			m_tangentX[i*n + j] = XMFLOAT3(1.0f, 0.0f, 0.0f);
		}
	}
}

LitWave::~LitWave()
{
	delete[] m_prevSolution;
	delete[] m_currSolution;
	delete[] m_normals;
	delete[] m_tangentX;
}

void LitWave::BuildShape()
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
			vertices[i].normal = XMFLOAT3(0.0f, 1.0f, 0.0f);
			//vertices[i].color = XMFLOAT4(Colors::Red);
		}

		D3D11_BUFFER_DESC vbDesc2;
		vbDesc2.ByteWidth = sizeof(VertexType) * vertexCount;;
		vbDesc2.Usage = D3D11_USAGE_IMMUTABLE;
		vbDesc2.BindFlags = D3D11_BIND_VERTEX_BUFFER;
		vbDesc2.CPUAccessFlags = 0;
		vbDesc2.MiscFlags = 0;
		vbDesc2.StructureByteStride = 0;

		D3D11_SUBRESOURCE_DATA vbInitData;
		vbInitData.pSysMem = vertices;
		vbInitData.SysMemPitch = 0;
		vbInitData.SysMemSlicePitch = 0;

		HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc2, &vbInitData, m_vertexBuffer.GetAddressOf());
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
	cbDesc.ByteWidth = sizeof(cbPerObjectStruct);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	hr = m_d3dDevice->CreateBuffer(&cbDesc, nullptr, m_constantBufferPerObject.GetAddressOf());
	DX::ThrowIfFailed(hr);

	delete[] indices;

	time_t t;
	srand((unsigned)time(&t));
}

void LitWave::BuildMaterial()
{
	m_cbPerObject.material.ambient = XMFLOAT4(0.137f, 0.42f, 0.556f, 1.0f);
	m_cbPerObject.material.diffuse = XMFLOAT4(0.137f, 0.42f, 0.556f, 0.5f);
	m_cbPerObject.material.specular = XMFLOAT4(0.8f, 0.8f, 0.8f, 96.0f);
	m_cbPerObject.material.reflect = XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f);
}

void LitWave::Update(DX::StepTimer const & timer)
{
	float elapsedTime = float(timer.GetElapsedSeconds());
	//
	// Every quarter second, generate a random wave.
	//

	static float t_base = 0.0f;
	if ((timer.GetTotalSeconds() - t_base) >= 0.25f)
	{
		t_base += 0.25f;

		DisturbWave();
	}

	//
	// Update waves
	//

	UpdateWave(elapsedTime);

	//
	// Update the wave vertex buffer with the new solution.
	//

	D3D11_MAPPED_SUBRESOURCE mappedData;
	HRESULT hr = m_d3dContext->Map(m_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);

	VertexType* v = reinterpret_cast<VertexType*>(mappedData.pData);

	const UINT vertexCount = m_numRows * m_numCols;
	const float width = m_numCols * m_spatialStep;
	const float depth = m_numRows * m_spatialStep;
	float totalTime = timer.GetTotalSeconds();
	float rateU = 0.05f;
	float rateV = 0.02f;
	for (UINT i = 0; i < vertexCount; ++i)
	{
		v[i].position = m_currSolution[i];
#ifdef USE_VERTEX_COLOR
		v[i].color = XMFLOAT4(Colors::Blue);
#else
		v[i].textureUV.x = 0.5 + m_currSolution[i].x / width + rateU * totalTime;
		v[i].textureUV.y = 0.5 - m_currSolution[i].z / depth + rateV * totalTime;

#endif
		v[i].normal = m_normals[i];
	}

	m_d3dContext->Unmap(m_vertexBuffer.Get(), 0);

	Super::Update(timer);
}

void LitWave::DisturbWave()
{
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

void LitWave::UpdateWave(float dt)
{
	static float t = 0;

	// Accumulate time.
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

		//
		// Compute normals using finite difference scheme.
		//
		for (UINT i = 1; i < m_numRows - 1; ++i)
		{
			for (UINT j = 1; j < m_numCols - 1; ++j)
			{
				float l = m_currSolution[i*m_numCols + j - 1].y;
				float r = m_currSolution[i*m_numCols + j + 1].y;
				float t = m_currSolution[(i - 1)*m_numCols + j].y;
				float b = m_currSolution[(i + 1)*m_numCols + j].y;
				m_normals[i*m_numCols + j].x = -r + l;
				m_normals[i*m_numCols + j].y = 2.0f*m_spatialStep;
				m_normals[i*m_numCols + j].z = b - t;

				XMVECTOR n = XMVector3Normalize(XMLoadFloat3(&m_normals[i*m_numCols + j]));
				XMStoreFloat3(&m_normals[i*m_numCols + j], n);

				m_tangentX[i*m_numCols + j] = XMFLOAT3(2.0f*m_spatialStep, r - l, 0.0f);
				XMVECTOR T = XMVector3Normalize(XMLoadFloat3(&m_tangentX[i*m_numCols + j]));
				XMStoreFloat3(&m_tangentX[i*m_numCols + j], T);
			}
		}
	}
}

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV

void LitHill::BuildTexture()
{
	BuildTextureByName(L"TransparentWaveGame\\grass.dds", m_diffuseMapView);
}

void LitWave::BuildTexture()
{
	BuildTextureByName(L"TransparentWaveGame\\water1.dds", m_diffuseMapView);
}

#endif