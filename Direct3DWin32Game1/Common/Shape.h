#pragma once
#include "Common/RenderObject.h"

class Shape : public RenderObject
{
	using Super = RenderObject;

public:

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj) override;

	virtual void Update(DX::StepTimer const& timer) override;
	virtual void Render() override;

	virtual void UpdateConstantBufferPerObject();

	void SetVSAndPSShader(Microsoft::WRL::ComPtr<ID3D11VertexShader>& VS, Microsoft::WRL::ComPtr<ID3D11PixelShader>& PS);
	Microsoft::WRL::ComPtr<ID3D11VertexShader>& GetVSShader() { return m_vertexShader; }
	Microsoft::WRL::ComPtr<ID3D11PixelShader>& GetPSShader() { return m_pixelShader; }

protected:

	virtual void BuildShader();
	virtual void SetInputLayout();
	virtual void BuildShape() = 0;
	virtual void BuildConstantBuffer() = 0;

	Microsoft::WRL::ComPtr<ID3DBlob> m_VSByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> m_PSByteCode;

	void CreateConstantBufferPerObject(UINT bufferSize);

	void CreateVSAndPSShader(const std::wstring& vsFilename, const std::wstring& psFilename, const D3D_SHADER_MACRO* defines = nullptr);
};