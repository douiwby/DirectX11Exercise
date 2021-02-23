#pragma once
#include "TransparentWaveGame/TransparentWaveGame.h"

class BasicTessellationGame : public TransparentWaveGame
{
	using Super = TransparentWaveGame;

public:

	BasicTessellationGame()
	{
		m_maxRadius = 700.f;
		m_mouseMoveRate = 30.f;
	}

	virtual void Initialize(HWND window, int width, int height) override;

protected:

	virtual void AddObjects() override;
};

class TessellationHill : public LitShape
{
	using Super = LitShape;

public:

	virtual void Render() override;

protected:

	virtual void BuildShape();
	virtual void BuildMaterial();
	virtual void BuildShader();
	virtual void BuildTexture();

	float width = 150.f;
	float depth = 150.f;
	const UINT m = 50;
	const UINT n = 50;

	Microsoft::WRL::ComPtr<ID3DBlob> m_HSByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> m_DSByteCode;
	Microsoft::WRL::ComPtr<ID3D11HullShader> m_hullShader;
	Microsoft::WRL::ComPtr<ID3D11DomainShader> m_domainShader;
};