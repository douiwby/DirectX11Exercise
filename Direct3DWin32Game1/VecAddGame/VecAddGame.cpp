#include "pch.h"
#include "VecAddGame/VecAddGame.h"
#include "Common/d3dUtil.h"
#include <vector>
#include <fstream>

using namespace DirectX;
using Microsoft::WRL::ComPtr;

struct DataType
{
	XMFLOAT3 v1;
	XMFLOAT2 v2;
};

void VecAddGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	BuildBufferAndView();
	BuildShader();
	DoComputeWork();
}

void VecAddGame::BuildBufferAndView()
{
	std::vector<DataType> data1(m_numElements);
	std::vector<DataType> data2(m_numElements);

	for (int i = 0; i < m_numElements; ++i)
	{
		data1[i].v1 = XMFLOAT3(i, i, i);
		data1[i].v2 = XMFLOAT2(i, 0);

		data2[i].v1 = XMFLOAT3(-i, i, 0.0f);
		data2[i].v2 = XMFLOAT2(0, -i);
	}

	UINT byteWidth = m_numElements * sizeof(DataType);
	CD3D11_BUFFER_DESC dataDesc(byteWidth, D3D11_BIND_SHADER_RESOURCE);
	dataDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	dataDesc.StructureByteStride = sizeof(DataType);

	D3D11_SUBRESOURCE_DATA initData1;
	initData1.pSysMem = data1.data();
	initData1.SysMemPitch = 0;
	initData1.SysMemSlicePitch = 0;

	D3D11_SUBRESOURCE_DATA initData2;
	initData2.pSysMem = data2.data();
	initData2.SysMemPitch = 0;
	initData2.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&dataDesc, &initData1, m_dataBuffer1.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreateBuffer(&dataDesc, &initData2, m_dataBuffer2.GetAddressOf());
	DX::ThrowIfFailed(hr);

	CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_BUFFER);
	srvDesc.Buffer.NumElements = m_numElements;

	hr = m_d3dDevice->CreateShaderResourceView(m_dataBuffer1.Get(), &srvDesc, m_dataView1.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreateShaderResourceView(m_dataBuffer2.Get(), &srvDesc, m_dataView2.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// !!!!!!!!!!!!!!
	// A D3D11_USAGE_STAGING Resource cannot be bound to any parts of the graphics pipeline
	// Resource can't be D3D11_BIND_UNORDERED_ACCESS and D3D11_USAGE_STAGING at the same time

	CD3D11_BUFFER_DESC outputGpuDataDesc(byteWidth, D3D11_BIND_UNORDERED_ACCESS);
	outputGpuDataDesc.Usage = D3D11_USAGE_DEFAULT;
	outputGpuDataDesc.CPUAccessFlags = 0;
	outputGpuDataDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	outputGpuDataDesc.StructureByteStride = sizeof(DataType);

	hr = m_d3dDevice->CreateBuffer(&outputGpuDataDesc, nullptr, m_outputGpuBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	CD3D11_BUFFER_DESC outputCpuDataDesc(byteWidth, 0);
	outputCpuDataDesc.Usage = D3D11_USAGE_STAGING;
	outputCpuDataDesc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
	outputCpuDataDesc.MiscFlags = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED;
	outputCpuDataDesc.StructureByteStride = sizeof(DataType);

	hr = m_d3dDevice->CreateBuffer(&outputCpuDataDesc, nullptr, m_outputCpuBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	CD3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc(D3D11_UAV_DIMENSION_BUFFER);
	uavDesc.Buffer.NumElements = m_numElements;

	hr = m_d3dDevice->CreateUnorderedAccessView(m_outputGpuBuffer.Get(), &uavDesc, m_outputView.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void VecAddGame::BuildShader()
{
	ComPtr<ID3DBlob> mcsByteCode = d3dUtil::CompileShader(L"VecAddGame\\VecAdd.hlsl", nullptr, "CS", "cs_5_0");

	HRESULT hr = m_d3dDevice->CreateComputeShader(mcsByteCode->GetBufferPointer(), mcsByteCode->GetBufferSize(), nullptr, m_computeShader.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void VecAddGame::DoComputeWork()
{
	m_d3dContext->CSSetShaderResources(0, 1, m_dataView1.GetAddressOf());
	m_d3dContext->CSSetShaderResources(1, 1, m_dataView2.GetAddressOf());
	m_d3dContext->CSSetUnorderedAccessViews(0, 1, m_outputView.GetAddressOf(), nullptr);
	m_d3dContext->CSSetShader(m_computeShader.Get(), nullptr, 0);

	int numThreadGroup = ceil(float(m_numElements) / 32);
	m_d3dContext->Dispatch(numThreadGroup, 1, 1);

	m_d3dContext->CopyResource(m_outputCpuBuffer.Get(), m_outputGpuBuffer.Get());

	std::ofstream file("result.txt");
	D3D11_MAPPED_SUBRESOURCE mappedData;
	m_d3dContext->Map(m_outputCpuBuffer.Get(), 0, D3D11_MAP_READ, 0, &mappedData);
	DataType* outputData = reinterpret_cast<DataType*>(mappedData.pData);
	for (int i = 0; i < m_numElements; ++i)
	{
		file << outputData[i].v1.x << ',' << outputData[i].v1.y << ',' << outputData[i].v1.z << ','
			<< outputData[i].v2.x << ',' << outputData[i].v2.y << std::endl;
	}
	m_d3dContext->Unmap(m_outputCpuBuffer.Get(), 0);

	file.close();
}
