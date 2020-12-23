#pragma once
#include "MultiObjectGame/Shape.h"
#include "MultiObjectGame/VertexStructuer.h"

class LitShape : public Shape
{
	using Super = Shape;
public:

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj);

	virtual void Update(DX::StepTimer const& timer);
	virtual void Render();

protected:

	virtual void BuildShader();
	virtual void BuildMaterial();

	struct cbPerObjectStruct
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 worldInvTranspose;
		DirectX::XMFLOAT4X4 worldViewProj;
		Material material;
	} m_cbPerObject;
};