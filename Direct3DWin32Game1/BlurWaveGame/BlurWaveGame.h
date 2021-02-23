#pragma once
#include "TransparentWaveGame/TransparentWaveGame.h"

class BlurWaveGame : public TransparentWaveGame
{
	using Super = TransparentWaveGame;

public:

	virtual void Initialize(HWND window, int width, int height) override;

	virtual void OnWindowSizeChanged(int width, int height) override;

protected:

	virtual void Update(DX::StepTimer const& timer) override;
	virtual void PreObjectsRender() override;
	virtual void PostObjectsRender() override;

	void BuildCSShader();
	void BuildCSConstantBuffer();
	void BuildOffscreenViews();

	void UpdateBlurCount();

	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_horzBlurCS;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_vertBlurCS;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> m_offscreenTexture;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_offscreenRTV;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_offscreenSRV;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_offscreenUAV;
	Microsoft::WRL::ComPtr<ID3D11DepthStencilView> m_offscreenDepthStencilView;

	struct BlurSettings
	{
		// We cannot have an array entry in a constant buffer that gets mapped onto
		// root constants, so list each element.  

		int gBlurRadius;

		// Support up to 11 blur weights.
		float w0;
		float w1;
		float w2;
		float w3;
		float w4;
		float w5;
		float w6;
		float w7;
		float w8;
		float w9;
		float w10;
	} m_blurSettings;
	static const int MaxBlurRadius = 5;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_cbBlurSettings;

	int m_blurCount = 1;

private:

	std::vector<float> BlurWaveGame::CalcGaussWeights(float sigma);
};