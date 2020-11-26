#pragma once

#include "Game.h"

class InitGame
{
public:

	InitGame();
	~InitGame() = default;

	InitGame(InitGame const&) = delete;
	InitGame& operator= (InitGame const&) = delete;

	virtual void Initialize(HWND window, int width, int height);

	virtual void Tick();

	// Messages
	virtual void OnActivated();
	virtual void OnDeactivated();
	virtual void OnSuspending();
	virtual void OnResuming();
	virtual void OnWindowSizeChanged(int width, int height);

	// Properties
	void GetDefaultSize(int& width, int& height) const noexcept;

protected:

	virtual void Update(DX::StepTimer const& timer);
	virtual void Render();

	virtual void CreateDevice();
	virtual void CreateResources();

	virtual void Clear();
	virtual void Present();

	virtual void OnDeviceLost();

	// Rendering loop timer.
	DX::StepTimer                                   m_timer;

	// Device resources.
	HWND                                            m_window;
	int                                             m_outputWidth;
	int                                             m_outputHeight;

	D3D_FEATURE_LEVEL                               m_featureLevel;
	Microsoft::WRL::ComPtr<ID3D11Device>			m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		m_d3dContext;
	Microsoft::WRL::ComPtr<IDXGISwapChain>			m_swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView>  m_renderTargetView;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView>  m_depthStencilView;

	bool bEnable4xMsaa = true;
};