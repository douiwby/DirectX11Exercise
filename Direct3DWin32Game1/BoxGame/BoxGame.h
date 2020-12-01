#pragma once
#include "InitGame\InitGame.h"
#include "Common\d3dUtil.h"

struct BoxGameVertex
{
	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT4 color;
};

class BoxGame : public InitGame
{
public:
	using Super = InitGame;

	virtual void Initialize(HWND window, int width, int height);

protected:
	virtual void Update(DX::StepTimer const& timer);
	virtual void Render();

	virtual void SetInputLayout();
	void BuildBox();

	Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

	UINT m_indexCount;
};