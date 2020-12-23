#pragma once
#include "MultiObjectGame/RenderObject.h"

class Shape : public RenderObject
{
public:
	using Super = RenderObject;

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj);

	virtual void Update(DX::StepTimer const& timer);
	virtual void Render();

protected:

	virtual void BuildShader();
	virtual void SetInputLayout();
	virtual void BuildShape() = 0;

	Microsoft::WRL::ComPtr<ID3DBlob> m_VSByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> m_PSByteCode;

	virtual void CreateVSAndPSShader(const std::wstring& vsFilename, const std::wstring& psFilename);
};