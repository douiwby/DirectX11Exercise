#pragma once

#include "StepTimer.h"

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

	inline float AspectRatio() const;

	virtual void OnMouseDown(WPARAM btnState, int x, int y);
	virtual void OnMouseUp(WPARAM btnState, int x, int y);
	virtual void OnMouseMove(WPARAM btnState, int x, int y);
	virtual void OnMouseWheelMove(WPARAM wParam);

	virtual void ToggleWireframe();
	virtual void ToggleAutoRotate();
	virtual void ToggleCameraMode();

	virtual void OnKeyButtonPressed(WPARAM key);
	virtual void OnKeyButtonReleased(WPARAM key);

protected:

	virtual void Update(DX::StepTimer const& timer);
	virtual void Render();

	virtual void CreateDevice();
	virtual void CreateResources();

	virtual void Clear();
	virtual void Present();

	virtual void OnDeviceLost();

	virtual void CalculateFrameStats();

	virtual void UpdateCamera();

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

	float m_nearZ = 1.f;
	float m_farZ = 1000.f;

	float m_fovAngleY = 45.f * DirectX::XM_PI / 180.f;

	DirectX::XMFLOAT4 m_eyePos;

	DirectX::XMFLOAT4X4 m_world;
	DirectX::XMFLOAT4X4 m_view;
	DirectX::XMFLOAT4X4 m_proj;

	bool bEnable4xMsaa = true;	
	bool bAutoRotate = false;
	bool bShowWireframe = false;

	POINT m_lastMousePos;
	bool bUsingOrbitCamera = false;

	// Free camera
	float m_theta;
	float m_phi;
	float m_radius;
	float m_minRadius = 1.0f;
	float m_maxRadius = 25.f;
	float m_mouseMoveRate = 1.f;
	float m_initCameraY = 2.f;
	float m_initCameraZ = -5.f;
	float m_rotateSpeed = DirectX::XMConvertToRadians(45);

	// Orbit camera
	DirectX::XMVECTOR m_pos;
	DirectX::XMVECTOR m_target;
	DirectX::XMVECTOR m_up;
	DirectX::XMVECTOR m_dir;
	bool bCameraMoveMode = false;
	bool bCameraShouldMoveUp = false;
	bool bCameraShouldMoveDown = false;
	bool bCameraShouldMoveRight = false;
	bool bCameraShouldMoveLeft = false;
	bool bCameraShouldMoveForward = false;
	bool bCameraShouldMoveBack = false;
	float m_cameraMoveRate = 10.f;
	float m_rotateCameraRate = DirectX::XMConvertToRadians(10);
};