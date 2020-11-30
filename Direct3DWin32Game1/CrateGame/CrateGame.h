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
	using Super = InitGame;

	virtual void Initialize(HWND window, int width, int height);

protected:

	virtual void SetInputLayout();
	void BuildCrate();
	void BuildTexture();

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_diffuseMapView;
};