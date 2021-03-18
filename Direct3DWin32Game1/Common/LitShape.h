#pragma once
#include "Common/Shape.h"
#include "Common/VertexStructuer.h"

//#define USE_VERTEX_COLOR 1
#define USE_TEXTURE_UV 1

class LitShape : public Shape
{
	using Super = Shape;

public:

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj) override;

	virtual void Render() override;

	virtual void UpdateConstantBufferPerObject() override;

protected:

	virtual void BuildShader() override;
	virtual void SetInputLayout() override;
	virtual void BuildConstantBuffer() override;
	virtual void BuildMaterial();

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
	virtual void BuildTexture() = 0;
	virtual void BuildTextureByName(const wchar_t * fileName, Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& textureView);
#endif

	struct cbPerObjectStruct
	{
		DirectX::XMFLOAT4X4 world;
		DirectX::XMFLOAT4X4 worldInvTranspose;
		DirectX::XMFLOAT4X4 worldViewProj;
		Material material;
	} m_cbPerObject;

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_diffuseMapView;
#endif
};