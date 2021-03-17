#pragma once
#include "Common/d3dUtil.h"
#include "StepTimer.h"

class RenderObject
{
public:
	RenderObject() = default;
	virtual ~RenderObject();

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device, 
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj);

	virtual void Update(DX::StepTimer const& timer) = 0;
	virtual void Render() = 0;

	void WorldTransform(DirectX::XMMATRIX& trans);

protected:

	Microsoft::WRL::ComPtr<ID3D11Device>			m_d3dDevice;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>		m_d3dContext;

	Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;

	Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBufferPerObject;
	
	DirectX::XMFLOAT4X4* m_world;
	DirectX::XMFLOAT4X4* m_view;
	DirectX::XMFLOAT4X4* m_proj;

	UINT m_indexCount;
};