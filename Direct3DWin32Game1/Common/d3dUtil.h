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
}