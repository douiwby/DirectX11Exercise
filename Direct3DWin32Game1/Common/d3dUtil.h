//***************************************************************************************
// d3dUtil.h by Frank Luna (C) 2015 All Rights Reserved.
//
// General helper code.
//***************************************************************************************

#pragma once

#include <windows.h>
#include <wrl.h>
#include <string>
#include <D3Dcompiler.h>

namespace d3dUtil
{
	Microsoft::WRL::ComPtr<ID3DBlob> CompileShader(
		const std::wstring& filename,
		const D3D_SHADER_MACRO* defines,
		const std::string& entrypoint,
		const std::string& target);

	template<typename T1, typename T2>
	inline void UpdateDynamicBufferFromData(Microsoft::WRL::ComPtr<ID3D11DeviceContext>& d3dContext, T1& buffer, T2& data)
	{
		D3D11_MAPPED_SUBRESOURCE mappedData;
		HRESULT hr = d3dContext->Map(buffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedData);
		DX::ThrowIfFailed(hr);
		auto bufferData = reinterpret_cast<T2*>(mappedData.pData);
		*bufferData = data;
		d3dContext->Unmap(buffer.Get(), 0);
	}
}