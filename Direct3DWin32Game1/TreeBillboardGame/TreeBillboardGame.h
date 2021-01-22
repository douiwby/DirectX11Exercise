#pragma once
#include "TransparentWaveGame/TransparentWaveGame.h"

class TreeBillboardGame : public TransparentWaveGame
{
	using Super = TransparentWaveGame;

public:

	virtual void Initialize(HWND window, int width, int height) override;

protected:

	virtual void AddObjects() override;
};

class TreeBillboard : public LitShape
{
	using Super = LitShape;

public:

	virtual void Update(DX::StepTimer const& timer) override;

	virtual void Render() override;

protected:

	virtual void BuildShader() override;
	virtual void SetInputLayout() override;
	virtual void BuildShape() override;
	virtual void BuildMaterial() override;

	virtual void BuildTexture() override;

	inline float GetHeight(float x, float z) const;

	Microsoft::WRL::ComPtr<ID3DBlob> m_GSByteCode;
	Microsoft::WRL::ComPtr<ID3D11GeometryShader> m_geometryShader;

	static const UINT m_treeCount = 20;
	const float m_BillboardWidth = 24.f;
	const float m_BillboardHeight = 24.f;
	const float m_ForestHalfRange = 75.f;

	bool m_AlphaToCoverageOn = true;
};