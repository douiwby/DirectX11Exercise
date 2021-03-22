#pragma once
#include "TransparentWaveGame/TransparentWaveGame.h"

class SkyGame : public TransparentWaveGame
{
	using Super = TransparentWaveGame;

public:

	SkyGame()
	{
		m_maxRadius = 1000.f;
	}

	virtual void Initialize(HWND window, int width, int height) override;

protected:

	virtual void Update(DX::StepTimer const& timer) override;

	virtual void AddObjects() override;

	virtual void PreObjectsRender() override;

	virtual void PostObjectsRender() override;

	void BuildDynamicCubeMapViews();

	void RenderDynamicCubeMapTexture();

	void GenerateViewMatrixByIndex(float x, float y, float z, int i, DirectX::XMMATRIX& view);

	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_dynamicCubeMapTexture;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_dynamicCubeMapSRV;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_dynamicCubeMapRTV[6];
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_dynamicCubeMapDSV;

	class ReflectBox* m_reflectObject;

	UINT m_cubeMapSize = 256;
};

class SkyBox : public LitShape
{
	using Super = LitShape;

public:

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj) override;

	virtual void Render() override;

protected:

	virtual void BuildShape() override;
	virtual void BuildShader() override;
	virtual void BuildConstantBuffer() override;

	virtual void BuildTexture();

	virtual void SetInputLayout() override;

	float m_length = 5000.f;
	float m_width = 5000.f;
	float m_height = 5000.f;

	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilState> m_depthStencilState;
};

class SkySphere : public SkyBox
{
	using Super = SkyBox;
	friend class SkyGame;

protected:

	virtual void BuildShape() override;

};

class ReflectBox : public Crate
{
	using Super = Crate;
	friend class SkyGame;

public:

	ReflectBox()
	{
		m_scale = 10.f;
	}

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj) override;

	virtual void Render() override;

protected:

	virtual void BuildMaterial();
	virtual void BuildShader();
	virtual void BuildTexture();

	virtual void RenderInternal(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& cubeMapSRV);

	bool bUsingDynamicCubeMap = false;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_cubeMapView;
};

class ReflectSphere : public ReflectBox
{
	using Super = ReflectBox;
	friend class SkyGame;

protected:

	virtual void BuildShape() override;
};