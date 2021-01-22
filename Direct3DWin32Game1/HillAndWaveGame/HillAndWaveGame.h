#pragma once
#include "Common/Shape.h"
#include "MultiObjectGame/MultiObjectGame.h"

class HillAndWaveGame : public MultiObjectGame
{
	using Super = MultiObjectGame;

public:

	virtual ~HillAndWaveGame();

	virtual void Initialize(HWND window, int width, int height) override;

protected:

	virtual void AddObjects() override;
};

class Hill : public Shape
{
	using Super = Shape;

protected:
	virtual void BuildShape();
	virtual void BuildConstantBuffer();

	inline float GetHeight(float x, float z) const;
	inline DirectX::XMFLOAT3 GetHillNormal(float x, float z) const;

	float width = 150.f;
	float depth = 150.f;
	const UINT m = 50;
	const UINT n = 50;
};

class Wave : public Shape
{
	using Super = Shape;
public:
	Wave();
	virtual ~Wave();

	virtual void Update(DX::StepTimer const& timer);

protected:	
	virtual void BuildShape();
	virtual void BuildConstantBuffer();

	DirectX::XMFLOAT3* m_prevSolution;
	DirectX::XMFLOAT3* m_currSolution;
	
	float mK1;
	float mK2;
	float mK3;

	float m_speed = 3.25f;
	float m_damping = 0.4f;

	float m_timeStep = 0.03f;
	float m_spatialStep = 0.75f;

	const UINT m_numRows = 201;
	const UINT m_numCols = 201;

};