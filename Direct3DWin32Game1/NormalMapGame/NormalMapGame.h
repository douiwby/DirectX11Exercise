#pragma once
#include "SkyGame/SkyGame.h"

class NormalMapGame : public SkyGame
{
	using Super = SkyGame;

public:

	virtual void Initialize(HWND window, int width, int height) override;

	virtual void OnKeyButtonReleased(WPARAM key) override;

protected:

	virtual void AddObjects() override;

	class Cylinder* m_cylinder = nullptr;

	bool bUsingNormalMap = true;
};

class NormalMapShape : public LitShape
{
	using Super = LitShape;

public:

	virtual void Render() override;

protected:

	virtual void BuildShader() override;
	virtual void SetInputLayout() override;

	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_normalMapView;
};

class Cylinder : public NormalMapShape
{
	using Super = NormalMapShape;
	friend class NormalMapGame;

public:

protected:

	virtual void BuildShader() override;

	virtual void BuildMaterial() override;

	virtual void BuildShape() override;

	virtual void BuildTexture() override;
};