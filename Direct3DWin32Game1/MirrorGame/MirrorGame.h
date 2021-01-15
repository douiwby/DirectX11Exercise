#pragma once
#include "MultiObjectGame/MultiObjectGame.h"
#include "Common/LightStructuer.h"
#include "Common/LitShape.h"

class MirrorGame : public MultiObjectGame
{
	using Super = MultiObjectGame;

public:

	virtual ~MirrorGame();

	virtual void Initialize(HWND window, int width, int height);

protected:

	virtual void BuildLight();

	virtual void Update(DX::StepTimer const& timer);

	virtual void UpdateLightPosition(DX::StepTimer const& timer);

	virtual void PreObjectsRender();

	virtual void PostObjectsRender();

	struct cbPerFrame
	{
		DirectionalLight dirLight;
		PointLight pointLight;
		SpotLight spotLight;
		DirectX::XMFLOAT4 eyePosW;
	} m_cbPerFrame;

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_constantBufferPerFrame;

	DirectionalLight m_dirLight;
	PointLight m_pointLight;
	SpotLight m_spotLight;

	RenderObject* mirror;
	std::vector<class ReflectShape*> m_reflectObjects;

private:

	inline void RotateVectorByZAxis(DirectX::XMFLOAT3& vector, float rotateRadian);
	inline void RotateVectorByXAxis(DirectX::XMFLOAT3& vector, float rotateRadian);
	inline void RotateVector(DirectX::XMFLOAT3& vector, DirectX::XMFLOAT4X4& rotation);
};

class ReflectShape : public LitShape
{
	using Super = LitShape;
	friend class MirrorGame;
};

class Wall : public ReflectShape
{
	using Super = ReflectShape;

public:

	virtual void Render();

protected:

	virtual void BuildShape();
	virtual void BuildMaterial();
	virtual void BuildShader();
	virtual void BuildTexture();

	float width = 10.f;
	float height = 5.f;
};

class Floor : public Wall
{
	using Super = Wall;

protected:

	virtual void BuildShape();
	virtual void BuildTexture();
};

class LitCrate : public ReflectShape
{
	using Super = ReflectShape;

protected:

	virtual void BuildShape();
	virtual void BuildMaterial();
	virtual void BuildShader();
	virtual void BuildTexture();

	float edgeLength = 2.f;
};

class Mirror : public Wall
{
	using Super = Wall;

public:

	Mirror() { width = 4.f; height = 3.f; };

protected:

	virtual void BuildShape();
	virtual void BuildTexture();
};