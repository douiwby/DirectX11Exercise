#pragma once
#include "NormalMapGame/NormalMapGame.h"

class ShadowGame : public NormalMapGame
{
	using Super = NormalMapGame;

public:
	ShadowGame()
	{
		m_initCameraY = 10.f;
		m_initCameraZ = -10.f;
	}


	~ShadowGame();

	virtual void Initialize(HWND window, int width, int height) override;

	virtual void OnKeyButtonReleased(WPARAM key) override;

protected:

	virtual void AddObjects() override;

	virtual void PreObjectsRender() override;

	virtual void PostObjectsRender() override;

	virtual void BuildConstantBuffer() override;

	virtual void UpdateConstantBufferPerFrame() override;

	void BuildShadowMapViews();

	void RenderShadowMapTexture();

	struct cbPerFrame
	{
		DirectionalLight dirLight;
		PointLight pointLight;
		SpotLight spotLight;
		DirectX::XMFLOAT4 eyePosW;
		DirectX::XMFLOAT4 fogColor;
		float fogStart;
		float fogRange;
		DirectX::XMFLOAT2 cbPerFramePad;
		DirectX::XMFLOAT4X4 viewProj;
		DirectX::XMFLOAT4X4 shadowTransform;
	} m_cbPerFrame;

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_shadowMapTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_shadowMapSRV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_shadowMapDSV;

	Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_shadowMapVertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_shadowMapPixelShader;

	class ScreenQuad* m_screenQuad = nullptr;

	// Choose to use what light as shadow source
	bool bUsingSpotOrDirectionalLight = true;
	bool bUsingDirectionalorPointLight = true;
};

class ScreenQuad : public LitShape
{
	using Super = LitShape;

public:

	virtual void UpdateConstantBufferPerObject() override;

	virtual void SetTransformMatrix(DirectX::XMMATRIX& m)
	{
		m_transformMatrix = m;
	}

	virtual void UpdateTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& inSRV);

protected:

	virtual void BuildShader() override;
	virtual void BuildTexture() override;
	virtual void BuildShape() override;
	virtual void BuildConstantBuffer() override;

	struct cbPerObjectStruct
	{
		DirectX::XMFLOAT4X4 worldViewProj;
	} m_cbPerObject;

	// The vertex's position value is already in homogeneous space, so default we set this to identity matrix
	DirectX::XMMATRIX m_transformMatrix = DirectX::XMMatrixIdentity();
};

class ShadowMapQuad : public ScreenQuad
{
	using Super = ScreenQuad;

public:

	ShadowMapQuad();

protected:

	virtual void BuildShader() override;
};