#include "pch.h"
#include "BlurWaveGame/BlurWaveGame.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void BlurWaveGame::Initialize(HWND window, int width, int height)
{
	bEnable4xMsaa = false; // Disable it to make sample count to be 1

	Super::Initialize(window, width, height);

	BuildCSShader();
	BuildCSConstantBuffer();
	BuildOffscreenViews();
}

void BlurWaveGame::OnWindowSizeChanged(int width, int height)
{
	Super::OnWindowSizeChanged(width, height);

	BuildOffscreenViews();
}

void BlurWaveGame::Update(DX::StepTimer const & timer)
{
	Super::Update(timer);

	UpdateBlurCount();
}

void BlurWaveGame::PreObjectsRender()
{
	// Render to off screen texture
	m_d3dContext->OMSetRenderTargets(1, m_offscreenRTV.GetAddressOf(), m_offscreenDepthStencilView.Get());
	m_d3dContext->ClearRenderTargetView(m_offscreenRTV.Get(), Colors::CornflowerBlue);
	m_d3dContext->ClearDepthStencilView(m_offscreenDepthStencilView.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
}

void BlurWaveGame::PostObjectsRender()
{
	// We must unbind the resource to be set as shader resource in CS
	m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	// Create an intermediate texture to store the first blur result
	ComPtr<ID3D11Texture2D> m_offscreenTextureIntermediate;
	ComPtr<ID3D11ShaderResourceView> m_offscreenSRVIntermediate;
	ComPtr<ID3D11UnorderedAccessView> m_offscreenUAVIntermediate;

	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	CD3D11_TEXTURE2D_DESC offscreenDesc(format, m_outputWidth, m_outputHeight);
	offscreenDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	HRESULT hr = m_d3dDevice->CreateTexture2D(&offscreenDesc, nullptr, m_offscreenTextureIntermediate.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreateShaderResourceView(m_offscreenTextureIntermediate.Get(), nullptr, m_offscreenSRVIntermediate.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreateUnorderedAccessView(m_offscreenTextureIntermediate.Get(), nullptr, m_offscreenUAVIntermediate.GetAddressOf());
	DX::ThrowIfFailed(hr);

	ID3D11ShaderResourceView* nullSRV[1] = { nullptr };
	ID3D11UnorderedAccessView* nullUAV[1] = { nullptr };

	m_d3dContext->CSSetConstantBuffers(0, 1, m_cbBlurSettings.GetAddressOf());

	for (int i = 0; i < m_blurCount; ++i)
	{
		// Horizontal blur
		m_d3dContext->CSSetShader(m_horzBlurCS.Get(), nullptr, 0);
		m_d3dContext->CSSetShaderResources(0, 1, m_offscreenSRV.GetAddressOf());
		m_d3dContext->CSSetUnorderedAccessViews(0, 1, m_offscreenUAVIntermediate.GetAddressOf(), nullptr);
		int numThreadGroupX = ceil(float(m_outputWidth) / 256);
		m_d3dContext->Dispatch(numThreadGroupX, m_outputHeight, 1);

		m_d3dContext->CSSetShaderResources(0, 1, nullSRV);
		m_d3dContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);

		// Vertical blur
		m_d3dContext->CSSetShader(m_vertBlurCS.Get(), nullptr, 0);
		m_d3dContext->CSSetShaderResources(0, 1, m_offscreenSRVIntermediate.GetAddressOf());
		m_d3dContext->CSSetUnorderedAccessViews(0, 1, m_offscreenUAV.GetAddressOf(), nullptr);
		int numThreadGroupY = ceil(float(m_outputHeight) / 256);
		m_d3dContext->Dispatch(m_outputWidth, numThreadGroupY, 1);

		m_d3dContext->CSSetShaderResources(0, 1, nullSRV);
		m_d3dContext->CSSetUnorderedAccessViews(0, 1, nullUAV, nullptr);
	}

	// Disable compute shader
	m_d3dContext->CSSetShader(nullptr, nullptr, 0);

	// Copy result to back buffer
	ComPtr<ID3D11Resource> backBuffer;
	m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer));
	m_d3dContext->CopyResource(backBuffer.Get(), m_offscreenTexture.Get());
}

void BlurWaveGame::BuildCSShader()
{
	ComPtr<ID3DBlob> mcsByteCode = d3dUtil::CompileShader(L"BlurWaveGame\\Blur.hlsl", nullptr, "HorzBlurCS", "cs_5_0");
	HRESULT hr = m_d3dDevice->CreateComputeShader(mcsByteCode->GetBufferPointer(), mcsByteCode->GetBufferSize(), nullptr, m_horzBlurCS.GetAddressOf());
	DX::ThrowIfFailed(hr);

	mcsByteCode = d3dUtil::CompileShader(L"BlurWaveGame\\Blur.hlsl", nullptr, "VertBlurCS", "cs_5_0");
	hr = m_d3dDevice->CreateComputeShader(mcsByteCode->GetBufferPointer(), mcsByteCode->GetBufferSize(), nullptr, m_vertBlurCS.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void BlurWaveGame::BuildCSConstantBuffer()
{
	auto weights = CalcGaussWeights(2.5f);
	m_blurSettings.gBlurRadius = (int)weights.size() / 2;
	int size = weights.size() > 11 ? 11 : weights.size();
	float* w = &m_blurSettings.w0;
	for (int i = 0; i < size; ++i)
	{
		w[i] = weights[i];
	}

	D3D11_BUFFER_DESC cbDesc;
	cbDesc.ByteWidth = sizeof(BlurSettings);
	cbDesc.Usage = D3D11_USAGE_DEFAULT;
	cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
	cbDesc.CPUAccessFlags = 0;
	cbDesc.MiscFlags = 0;
	cbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA cbInitData;
	cbInitData.pSysMem = &m_blurSettings;
	cbInitData.SysMemPitch = 0;
	cbInitData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&cbDesc, &cbInitData, m_cbBlurSettings.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void BlurWaveGame::BuildOffscreenViews()
{
	DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM;
	CD3D11_TEXTURE2D_DESC offscreenDesc(format, m_outputWidth, m_outputHeight);
	offscreenDesc.MipLevels = 1;
	offscreenDesc.SampleDesc.Count = 1; // This should be the same as back buffer
	offscreenDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;

	HRESULT hr = m_d3dDevice->CreateTexture2D(&offscreenDesc, nullptr, m_offscreenTexture.GetAddressOf());
	DX::ThrowIfFailed(hr);

	hr = m_d3dDevice->CreateRenderTargetView(m_offscreenTexture.Get(), nullptr, m_offscreenRTV.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreateShaderResourceView(m_offscreenTexture.Get(), nullptr, m_offscreenSRV.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreateUnorderedAccessView(m_offscreenTexture.Get(), nullptr, m_offscreenUAV.GetAddressOf());
	DX::ThrowIfFailed(hr);

	CD3D11_TEXTURE2D_DESC depthStencilDesc(DXGI_FORMAT_D24_UNORM_S8_UINT, m_outputWidth, m_outputHeight);
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL;
	ComPtr<ID3D11Texture2D> mDepthStencilBuffer;
	hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, mDepthStencilBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreateDepthStencilView(mDepthStencilBuffer.Get(), nullptr, m_offscreenDepthStencilView.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void BlurWaveGame::UpdateBlurCount()
{
	if (GetAsyncKeyState('0') & 0x8000)
		m_blurCount = 0;
	if (GetAsyncKeyState('1') & 0x8000)
		m_blurCount = 1;
	if (GetAsyncKeyState('2') & 0x8000)
		m_blurCount = 2;
	if (GetAsyncKeyState('3') & 0x8000)
		m_blurCount = 3;
	if (GetAsyncKeyState('4') & 0x8000)
		m_blurCount = 4;
	if (GetAsyncKeyState('5') & 0x8000)
		m_blurCount = 5;
}

std::vector<float> BlurWaveGame::CalcGaussWeights(float sigma)
{
	float twoSigma2 = 2.0f*sigma*sigma;

	// Estimate the blur radius based on sigma since sigma controls the "width" of the bell curve.
	// For example, for sigma = 3, the width of the bell curve is 
	int blurRadius = (int)ceil(2.0f * sigma);

	assert(blurRadius <= MaxBlurRadius);

	std::vector<float> weights;
	weights.resize(2 * blurRadius + 1);

	float weightSum = 0.0f;

	for (int i = -blurRadius; i <= blurRadius; ++i)
	{
		float x = (float)i;

		weights[i + blurRadius] = expf(-x * x / twoSigma2);

		weightSum += weights[i + blurRadius];
	}

	// Divide by the sum so all the weights add up to 1.0.
	for (int i = 0; i < weights.size(); ++i)
	{
		weights[i] /= weightSum;
	}

	return weights;
}
