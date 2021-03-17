#pragma once
#include "MultiObjectGame/MultiObjectGame.h"
#include "Common/LitShape.h"
#include "Common/VertexStructuer.h"
#include "Common/LightStructuer.h"

class LitHillGame : public MultiObjectGame
{
	using Super = MultiObjectGame;

public:
	LitHillGame() 
	{
		m_initCameraY = 150.f;
		m_initCameraZ = -150.f;
		m_maxRadius = 300.f;
		m_mouseMoveRate = 15.f;
	}

	virtual ~LitHillGame();

	virtual void Initialize(HWND window, int width, int height) override;

protected:

	virtual void Update(DX::StepTimer const& timer) override;

	virtual void AddObjects() override;

	virtual void BuildLight();

	virtual void BuildConstantBuffer();

	virtual void UpdateLightPosition(DX::StepTimer const& timer);

	inline void RotateVectorByZAxis(DirectX::XMFLOAT3& vector, float rotateRadian);
	inline void RotateVectorByXAxis(DirectX::XMFLOAT3& vector, float rotateRadian);
	inline void RotateVector(DirectX::XMFLOAT3& vector, DirectX::XMFLOAT4X4& rotation);

	void CreateConstantBufferPerFrame(UINT bufferSize);
	virtual void UpdateConstantBufferPerFrame();

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
};

class LitHill : public LitShape
{
	using Super = LitShape;

protected:

	virtual void BuildShape();
	virtual void BuildMaterial();

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
	virtual void BuildTexture();
#endif

	inline float GetHeight(float x, float z) const;
	inline DirectX::XMFLOAT3 GetHillNormal(float x, float z) const;

	float width = 150.f;
	float depth = 150.f;
	const UINT m = 50;
	const UINT n = 50;
	
};

class LitWave : public LitShape
{
	using Super = LitShape;

public:
	LitWave();
	virtual ~LitWave();

	virtual void Update(DX::StepTimer const& timer);

protected:
	virtual void BuildShape();
	virtual void BuildMaterial();

#if USE_VERTEX_COLOR
#elif USE_TEXTURE_UV
	virtual void BuildTexture();
#endif

	void DisturbWave();
	void UpdateWave(float dt);

	DirectX::XMFLOAT3* m_prevSolution;
	DirectX::XMFLOAT3* m_currSolution;
	DirectX::XMFLOAT3* m_normals;
	DirectX::XMFLOAT3* m_tangentX;

	float mK1;
	float mK2;
	float mK3;

	const float m_speed = 3.25f;
	const float m_damping = 0.4f;

	const float m_timeStep = 0.03f;
	const float m_spatialStep = 0.75f;

	const UINT m_numRows = 201;
	const UINT m_numCols = 201;
	
};