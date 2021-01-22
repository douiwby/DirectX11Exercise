#pragma once
#include "Common/RenderObject.h"

class Shape : public RenderObject
{
public:
	using Super = RenderObject;

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj) override;

	virtual void Update(DX::StepTimer const& timer) override;
	virtual void Render() override;

protected:

	virtual void BuildShader();
	virtual void SetInputLayout();
	virtual void BuildShape() = 0;
	virtual void BuildConstantBuffer() = 0;

	Microsoft::WRL::ComPtr<ID3DBlob> m_VSByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> m_PSByteCode;

	void CreateVSAndPSShader(const std::wstring& vsFilename, const std::wstring& psFilename, const D3D_SHADER_MACRO* defines = nullptr);
	void CreateConstantBuffer(UINT bufferSize);
};