#include "pch.h"
#include "InitGame.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

InitGame::InitGame():
	m_window(nullptr),
	m_outputWidth(800),
	m_outputHeight(600),
	m_featureLevel(D3D_FEATURE_LEVEL_11_0)
{
}

void InitGame::Initialize(HWND window, int width, int height)
{
	m_window = window;
	m_outputWidth = std::max(width, 1);
	m_outputHeight = std::max(height, 1);

	CreateDevice();

	CreateResources();
}

void InitGame::Tick()
{
	m_timer.Tick([&]()
	{
		Update(m_timer);
	});

	Render();
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

	// TODO: Game window is being resized.
}

// Properties
void InitGame::GetDefaultSize(int& width, int& height) const noexcept
{
	// TODO: Change to desired default window size (note minimum size is 320x200).
	width = 800;
	height = 600;
}

// Updates the world.
void InitGame::Update(DX::StepTimer const& timer)
{
	float elapsedTime = float(timer.GetElapsedSeconds());

	// TODO: Add your game logic here.
	elapsedTime;
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
		m_d3dDevice->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice);
		ComPtr<IDXGIAdapter> dxgiAdapter;
		dxgiDevice->GetParent(__uuidof(IDXGIAdapter), &dxgiAdapter);
		ComPtr<IDXGIFactory> dxgiFactory;
		dxgiAdapter->GetParent(__uuidof(IDXGIFactory), &dxgiFactory);

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
