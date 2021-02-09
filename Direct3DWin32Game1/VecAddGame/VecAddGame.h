#pragma once
#include "InitGame/InitGame.h"

class VecAddGame : public InitGame
{
	using Super = InitGame;

public:

	virtual void Initialize(HWND window, int width, int height) override;

protected:

	void BuildBufferAndView();

	void BuildShader();

	void DoComputeWork();

	int m_numElements = 1000;


	Microsoft::WRL::ComPtr<ID3D11Buffer> m_dataBuffer1;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_dataBuffer2;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_outputGpuBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer> m_outputCpuBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_dataView1;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_dataView2;
	Microsoft::WRL::ComPtr<ID3D11UnorderedAccessView> m_outputView;
	Microsoft::WRL::ComPtr<ID3D11ComputeShader> m_computeShader;
};