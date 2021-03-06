#include "pch.h"
#include "InitGame.h"
#include <sstream>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

InitGame::InitGame() :
	m_window(nullptr),
	m_outputWidth(800),
	m_outputHeight(600),
	m_featureLevel(D3D_FEATURE_LEVEL_11_0),
	m_theta(1.5f*XM_PI),
	m_phi(0.25f*XM_PI),
	m_radius(5.0f)
{
}

void InitGame::Initialize(HWND window, int width, int height)
{
	m_window = window;
	m_outputWidth = std::max(width, 1);
	m_outputHeight = std::max(height, 1);

	CreateDevice();

	CreateResources();

	XMStoreFloat4x4(&m_world, XMMatrixIdentity());

	m_pos = XMVectorSet(0.f, m_initCameraY, m_initCameraZ, 1.0f);
	m_target = XMVectorSet(0.f, 0.f, 0.f, 1.f);
	m_dir = XMVector3Normalize(m_target - m_pos);
	m_up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

	UpdateCamera();

	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(m_fovAngleY, AspectRatio(), m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_proj, perspectiveMatrix);

	//XMMATRIX orthographicMatrix = XMMatrixOrthographicLH(400.f, 400.f, m_nearZ, m_farZ);
	//XMStoreFloat4x4(&m_proj, orthographicMatrix);

	m_radius = sqrt(pow(m_initCameraY,2)+pow(m_initCameraZ,2));
}

void InitGame::Tick()
{
	m_timer.Tick([&]()
	{
		Update(m_timer);
	});

	Render();

	CalculateFrameStats();
}

// Message handlers
void InitGame::OnActivated()
{
	// TODO: Game is becoming active window.
}

void InitGame::OnDeactivated()
{
	// TODO: Game is becoming background window.
}

void InitGame::OnSuspending()
{
	// TODO: Game is being power-suspended (or minimized).
}

void InitGame::OnResuming()
{
	m_timer.ResetElapsedTime();

	// TODO: Game is being power-resumed (or returning from minimize).
}

void InitGame::OnWindowSizeChanged(int width, int height)
{
	m_outputWidth = std::max(width, 1);
	m_outputHeight = std::max(height, 1);

	CreateResources();

	XMMATRIX perspectiveMatrix = XMMatrixPerspectiveFovLH(m_fovAngleY, AspectRatio(), m_nearZ, m_farZ);
	XMStoreFloat4x4(&m_proj, perspectiveMatrix);

	// TODO: Game window is being resized.
}

// Properties
void InitGame::GetDefaultSize(int& width, int& height) const noexcept
{
	// TODO: Change to desired default window size (note minimum size is 320x200).
	width = 800;
	height = 600;
}

inline float InitGame::AspectRatio() const
{
	return static_cast<float>(m_outputWidth) / m_outputHeight;
}

// Updates the world.
void InitGame::Update(DX::StepTimer const& timer)
{
	float elapsedTime = float(timer.GetElapsedSeconds());

	if (bUsingOrbitCamera)
	{
		if (bAutoRotate)
		{
			m_theta += m_rotateSpeed * elapsedTime;
		}

		// Convert Spherical to Cartesian coordinates.
		float x = m_radius * sinf(m_phi)*cosf(m_theta);
		float z = m_radius * sinf(m_phi)*sinf(m_theta);
		float y = m_radius * cosf(m_phi);

		// Build the view matrix.
		m_pos = XMVectorSet(x, y, z, 1.0f);
		m_target = XMVectorZero();
	}
	else
	{
		XMVECTOR cameraRight = XMVector3Normalize(XMVector3Cross(m_up, m_dir));
		XMVECTOR cameraUp = XMVector3Normalize(XMVector3Cross(m_dir, cameraRight));
		XMVECTOR cameraForward = XMVector3Normalize(m_dir);

		if (bCameraShouldMoveUp)
		{
			m_pos = XMVector3TransformCoord(m_pos, XMMatrixTranslationFromVector(m_cameraMoveRate*elapsedTime*cameraUp));
		}
		else if (bCameraShouldMoveDown)
		{
			m_pos = XMVector3TransformCoord(m_pos, XMMatrixTranslationFromVector(-m_cameraMoveRate*elapsedTime*cameraUp));
		}

		if (bCameraShouldMoveRight)
		{
			m_pos = XMVector3TransformCoord(m_pos, XMMatrixTranslationFromVector(m_cameraMoveRate*elapsedTime*cameraRight));
		}
		else if (bCameraShouldMoveLeft)
		{
			m_pos = XMVector3TransformCoord(m_pos, XMMatrixTranslationFromVector(-m_cameraMoveRate*elapsedTime*cameraRight));
		}

		if (bCameraShouldMoveForward)
		{
			m_pos = XMVector3TransformCoord(m_pos, XMMatrixTranslationFromVector(m_cameraMoveRate*elapsedTime*cameraForward));
		}
		else if (bCameraShouldMoveBack)
		{
			m_pos = XMVector3TransformCoord(m_pos, XMMatrixTranslationFromVector(-m_cameraMoveRate*elapsedTime*cameraForward));
		}
	}

	UpdateCamera();
}

// Draws the scene.
void InitGame::Render()
{
	// Don't try to render anything before the first Update.
	if (m_timer.GetFrameCount() == 0)
	{
		return;
	}

	Clear();

	// TODO: Add your rendering code here.

	Present();
}

void InitGame::CreateDevice()
{
	// Create device and context
	UINT createDeviceFlag = 0;
#ifdef _DEBUG
	// Check for SDK Layer support.
	HRESULT hrDebug = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_NULL,       // There is no need to create a real hardware device.
		0,
		D3D11_CREATE_DEVICE_DEBUG,  // Check for the SDK layers.
		nullptr,                    // Any feature level will do.
		0,
		D3D11_SDK_VERSION,          // Always set this to D3D11_SDK_VERSION for Windows Store apps.
		nullptr,                    // No need to keep the D3D device reference.
		nullptr,                    // No need to know the feature level.
		nullptr                     // No need to keep the D3D device context reference.
	);
	bool bSdkLayersAvailable = SUCCEEDED(hrDebug);
	if (bSdkLayersAvailable)
	{
		// If the project is in a debug build, enable debugging via SDK Layers with this flag.
		createDeviceFlag |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	HRESULT hr = D3D11CreateDevice(
		nullptr,
		D3D_DRIVER_TYPE_HARDWARE,
		nullptr,
		createDeviceFlag,
		nullptr,
		0,
		D3D11_SDK_VERSION,
		m_d3dDevice.GetAddressOf(),
		&m_featureLevel,
		m_d3dContext.GetAddressOf()
	);
	DX::ThrowIfFailed(hr);
}

void InitGame::CreateResources()
{
	const DXGI_FORMAT backBufferFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	const UINT backBufferCount = 2;

	UINT m4xMsaaQuality;
	HRESULT hr = m_d3dDevice->CheckMultisampleQualityLevels(DXGI_FORMAT_R8G8B8A8_UNORM, 4, &m4xMsaaQuality);
	DX::ThrowIfFailed(hr);
	assert(m4xMsaaQuality > 0);

	m_renderTargetView.Reset();
	m_depthStencilView.Reset();

	if (m_swapChain)
	{
		HRESULT hr = m_swapChain->ResizeBuffers(backBufferCount, m_outputWidth, m_outputHeight, backBufferFormat, 0);

		if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
		{
			// If the device was removed for any reason, a new device and swap chain will need to be created.
			OnDeviceLost();

			// Everything is set up now. Do not continue execution of this method. OnDeviceLost will reenter this method 
			// and correctly set up the new device.
			return;
		}
		else
		{
			DX::ThrowIfFailed(hr);
		}
	}
	else
	{
		// Create swap chain
		ComPtr<IDXGIDevice> dxgiDevice;
		DX::ThrowIfFailed(m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice));
		ComPtr<IDXGIAdapter> dxgiAdapter;
		DX::ThrowIfFailed(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), &dxgiAdapter));
		ComPtr<IDXGIFactory> dxgiFactory;
		DX::ThrowIfFailed(dxgiAdapter->GetParent(__uuidof(IDXGIFactory), &dxgiFactory));

		DXGI_SWAP_CHAIN_DESC scDesc;
		scDesc.BufferDesc.Width = m_outputWidth;
		scDesc.BufferDesc.Height = m_outputHeight;
		scDesc.BufferDesc.RefreshRate.Numerator = 60;
		scDesc.BufferDesc.RefreshRate.Denominator = 1;
		scDesc.BufferDesc.Format = backBufferFormat;
		scDesc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
		scDesc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
		if (bEnable4xMsaa)
		{
			scDesc.SampleDesc.Count = 4;
			scDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
		}
		else
		{
			scDesc.SampleDesc.Count = 1;
			scDesc.SampleDesc.Quality = 0;
		}
		scDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
		scDesc.BufferCount = backBufferCount;
		scDesc.OutputWindow = m_window;
		scDesc.Windowed = true;
		scDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
		scDesc.Flags = 0;

		hr = dxgiFactory->CreateSwapChain(
			m_d3dDevice.Get(),
			&scDesc,
			m_swapChain.GetAddressOf()
		);
		DX::ThrowIfFailed(hr);
	}

	// Create render targer view 
	ComPtr<ID3D11Resource> backBuffer;
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	hr = m_d3dDevice->CreateRenderTargetView(backBuffer.Get(), 0, m_renderTargetView.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Create depth stencil view
	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = m_outputWidth;
	depthStencilDesc.Height = m_outputHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	if (bEnable4xMsaa)
	{
		depthStencilDesc.SampleDesc.Count = 4;
		depthStencilDesc.SampleDesc.Quality = m4xMsaaQuality - 1;
	}
	else
	{
		depthStencilDesc.SampleDesc.Count = 1;
		depthStencilDesc.SampleDesc.Quality = 0;
	}
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	ComPtr<ID3D11Texture2D> mDepthStencilBuffer;
	hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, mDepthStencilBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	hr = m_d3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, m_depthStencilView.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Set render target
	m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// Set viewport
	D3D11_VIEWPORT viewport;
	viewport.TopLeftX = 0;
	viewport.TopLeftY = 0;
	viewport.Width = static_cast<float>(m_outputWidth);
	viewport.Height = static_cast<float>(m_outputHeight);
	viewport.MinDepth = 0.f;
	viewport.MaxDepth = 1.f;

	m_d3dContext->RSSetViewports(1, &viewport);
}

void InitGame::Clear()
{
	// Clear the views.
	m_d3dContext->ClearRenderTargetView(m_renderTargetView.Get(), Colors::CornflowerBlue);
	m_d3dContext->ClearDepthStencilView(m_depthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);

	m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// Set the viewport.
	CD3D11_VIEWPORT viewport(0.0f, 0.0f, static_cast<float>(m_outputWidth), static_cast<float>(m_outputHeight));
	m_d3dContext->RSSetViewports(1, &viewport);
}

void InitGame::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = m_swapChain->Present(1, 0);

	// If the device was reset we must completely reinitialize the renderer.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		HRESULT hr = m_d3dDevice->GetDeviceRemovedReason();
		OnDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

void InitGame::OnDeviceLost()
{    
	// TODO: Add Direct3D resource cleanup here.

	m_depthStencilView.Reset();
	m_renderTargetView.Reset();
	m_swapChain.Reset();
	m_d3dContext.Reset();
	m_d3dDevice.Reset();

	CreateDevice();

	CreateResources();
}

void InitGame::CalculateFrameStats()
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
		SetWindowText(m_window, outs.str().c_str());

		// Reset for next average.
		frameCnt = 0;
		timeElapsed += 1.0f;
	}
}

void InitGame::UpdateCamera()
{
	XMStoreFloat4(&m_eyePos, m_pos);

	if (bUsingOrbitCamera)
	{
		XMMATRIX V = XMMatrixLookAtLH(m_pos, m_target, m_up);
		XMStoreFloat4x4(&m_view, V);
	}
	else
	{
		XMMATRIX V = XMMatrixLookToLH(m_pos, m_dir, m_up);
		XMStoreFloat4x4(&m_view, V);
	}
}

void InitGame::OnMouseDown(WPARAM btnState, int x, int y)
{
	m_lastMousePos.x = x;
	m_lastMousePos.y = y;

	SetCapture(m_window);

	if ((btnState & MK_RBUTTON) != 0)
	{
		bCameraMoveMode = true;
	}
}

void InitGame::OnMouseUp(WPARAM btnState, int x, int y)
{
	ReleaseCapture();

	if ((btnState & MK_RBUTTON) != 0)
	{
		bCameraMoveMode = false;
	}
}

void InitGame::OnMouseMove(WPARAM btnState, int x, int y)
{
	if (bUsingOrbitCamera)
	{
		if ((btnState & MK_LBUTTON) != 0)
		{
			// Make each pixel correspond to a quarter of a degree.
			float dx = XMConvertToRadians(0.25f*static_cast<float>(x - m_lastMousePos.x));
			float dy = XMConvertToRadians(0.25f*static_cast<float>(y - m_lastMousePos.y));

			// Update angles based on input to orbit camera around box.
			if (!bAutoRotate)
			{
				m_theta += dx;
			}
			m_phi += dy;

			// Restrict the angle mPhi.
			m_phi = DX::Clamp(m_phi, 0.1f, XM_PI - 0.1f);
		}
		else if ((btnState & MK_RBUTTON) != 0)
		{
			// Make each pixel correspond to 0.01 unit in the scene.
			float dx = 0.01f*static_cast<float>(x - m_lastMousePos.x);
			float dy = 0.01f*static_cast<float>(y - m_lastMousePos.y);

			// Update the camera radius based on input.
			m_radius += (dx - dy) * m_mouseMoveRate;

			// Restrict the radius.
			m_radius = DX::Clamp(m_radius, m_minRadius, m_maxRadius);
		}
	}
	else
	{
		if ((btnState & MK_RBUTTON) != 0)
		{
			// Make each pixel correspond to 0.01 unit in the scene.
			float dx = 0.01f*static_cast<float>(x - m_lastMousePos.x);
			float dy = 0.01f*static_cast<float>(y - m_lastMousePos.y);

			m_dir = XMVector3Normalize(XMVector3TransformNormal(m_dir, XMMatrixRotationY(dx*m_rotateCameraRate)));
			XMVECTOR cameraRight = XMVector3Normalize(XMVector3Cross(m_up, m_dir));
			m_dir = XMVector3Normalize(XMVector3TransformNormal(m_dir, XMMatrixRotationAxis(cameraRight,dy*m_rotateCameraRate)));
		}
	}

	m_lastMousePos.x = x;
	m_lastMousePos.y = y;
}

void InitGame::OnMouseWheelMove(WPARAM wParam)
{
	float delta = GET_WHEEL_DELTA_WPARAM(wParam);
	if (delta > 0)
	{
		m_cameraMoveRate *= 2;
		float maximumMoveRate = 200.f;
		m_cameraMoveRate = m_cameraMoveRate > maximumMoveRate ? maximumMoveRate : m_cameraMoveRate;
	}
	else
	{
		m_cameraMoveRate *= 0.5;
		float lowestMoveRate = 0.5f;
		m_cameraMoveRate = m_cameraMoveRate < lowestMoveRate ? lowestMoveRate : m_cameraMoveRate;
	}
}

void InitGame::ToggleWireframe()
{
	bShowWireframe = !bShowWireframe;

	CD3D11_RASTERIZER_DESC rsDesc(D3D11_DEFAULT);
	if (bShowWireframe)
	{
		
		rsDesc.FillMode = D3D11_FILL_WIREFRAME;
		rsDesc.CullMode = D3D11_CULL_NONE;
	}
	ComPtr<ID3D11RasterizerState> mRSState;
	HRESULT hr = m_d3dDevice->CreateRasterizerState(&rsDesc, mRSState.GetAddressOf());
	DX::ThrowIfFailed(hr);
	m_d3dContext->RSSetState(mRSState.Get());
}

void InitGame::ToggleAutoRotate()
{
	bAutoRotate = !bAutoRotate;
}

void InitGame::ToggleCameraMode()
{
	bUsingOrbitCamera = !bUsingOrbitCamera;
}

void InitGame::OnKeyButtonPressed(WPARAM key)
{
	if (!bUsingOrbitCamera)
	{
		// Move camera when right mouse down
		if (GetAsyncKeyState(MK_RBUTTON) & 0x8000)
		{
			if (key == 'Q')
			{
				bCameraShouldMoveDown = true;
			}
			if (key == 'E')
			{
				bCameraShouldMoveUp = true;
			}
			if (key == 'D')
			{
				bCameraShouldMoveRight = true;
			}
			if (key == 'A')
			{
				bCameraShouldMoveLeft = true;
			}
			if (key == 'W')
			{
				bCameraShouldMoveForward = true;
			}
			if (key == 'S')
			{
				bCameraShouldMoveBack = true;
			}
		}
	}
}

void InitGame::OnKeyButtonReleased(WPARAM key)
{
	if (key == 'F')
	{
		ToggleWireframe();
	}

	if (key == 'G')
	{
		ToggleAutoRotate();
	}

	if (key == 'C')
	{
		ToggleCameraMode();
	}

	if (!bUsingOrbitCamera)
	{
		// Stop move camera when right mouse up
		{
			if (key == 'Q')
			{
				bCameraShouldMoveDown = false;
			}
			if (key == 'E')
			{
				bCameraShouldMoveUp = false;
			}
			if (key == 'D')
			{
				bCameraShouldMoveRight = false;
			}
			if (key == 'A')
			{
				bCameraShouldMoveLeft = false;
			}
			if (key == 'W')
			{
				bCameraShouldMoveForward = false;
			}
			if (key == 'S')
			{
				bCameraShouldMoveBack = false;
			}
		}

		// Reset the camera
		if (key == 'O')
		{
			m_pos = XMVectorSet(0.f, m_initCameraY, m_initCameraZ, 1.0f);
			m_target = XMVectorSet(0.f, 0.f, 0.f, 1.f);
			m_dir = XMVector3Normalize(m_target - m_pos);
			m_up = XMVectorSet(0.f, 1.f, 0.f, 0.f);

			UpdateCamera();
		}
	}
}
