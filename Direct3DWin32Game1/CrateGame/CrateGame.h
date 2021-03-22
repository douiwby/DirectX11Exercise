#pragma once
#include "BoxGame/BoxGame.h"

struct CrateGameVertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 normal;
	DirectX::XMFLOAT2 textureUV;
};

class CrateGame : public BoxGame
{
public:
	using Super = BoxGame;

	virtual void Initialize(HWND window, int width, int height) override;

	virtual void OnKeyButtonReleased(WPARAM key) override;

	virtual void ToggleSampler();

protected:

	virtual void SetInputLayout() override;
	void BuildCrate();
	void BuildTexture();

	int currentSampler = 0;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_diffuseMapView;
	Microsoft::WRL::ComPtr<ID3D11SamplerState> m_samplerState;
};