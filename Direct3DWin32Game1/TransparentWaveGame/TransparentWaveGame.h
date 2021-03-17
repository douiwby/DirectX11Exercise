#pragma once
#include "LitHillGame/LitHillGame.h"

class TransparentWaveGame : public LitHillGame
{
	using Super = LitHillGame;

public:

protected:

	virtual void BuildLight() override;

	virtual void BuildConstantBuffer() override;

	virtual void Update(DX::StepTimer const& timer) override;

	virtual void AddObjects() override;

	virtual void UpdateConstantBufferPerFrame() override;

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
	} m_cbPerFrame;
};

class TextureHill : public LitHill
{
	using Super = LitHill;

protected:

	virtual void BuildShader();
};

class TransparentWave : public LitWave
{
	using Super = LitWave;

public:

	virtual void Render();

protected:

	virtual void BuildShader();
};

class Crate : public LitShape
{
	using Super = LitShape;

public:

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj) override;

	virtual void Render();

protected:

	virtual void BuildShape();
	virtual void BuildMaterial();
	virtual void BuildShader();

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
	virtual void BuildTexture();
#endif

	float m_scale = 5.f;
};