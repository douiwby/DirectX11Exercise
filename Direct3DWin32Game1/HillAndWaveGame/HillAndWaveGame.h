#pragma once
#include "MultiObjectGame/RenderObject.h"
#include "MultiObjectGame/MultiObjectGame.h"
#include "MultiObjectGame/VertexStructuer.h"

class HillAndWaveGame : public MultiObjectGame
{
public:
	using Super = MultiObjectGame;

	virtual ~HillAndWaveGame();

	virtual void Initialize(HWND window, int width, int height);
};

class Shape : public RenderObject
{
public:
	using Super = RenderObject;

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj);

	virtual void Update(DX::StepTimer const& timer);
	virtual void Render();

protected:
	virtual void SetInputLayout();
	virtual void BuildShape() = 0;
};

class Hill : public Shape
{
public:
	using Super = Shape;

protected:
	virtual void BuildShape();

	inline float GetHeight(float x, float z) const;

	float width = 150.f;
	float depth = 150.f;
	const UINT m = 50;
	const UINT n = 50;
};

class Wave : public Shape
{
public:
	Wave();
	virtual ~Wave();
	using Super = Shape;

	virtual void Update(DX::StepTimer const& timer);

protected:	
	virtual void BuildShape();

	DirectX::XMFLOAT3* m_prevSolution;
	DirectX::XMFLOAT3* m_currSolution;
	
	float mK1;
	float mK2;
	float mK3;

	float m_speed = 3.25f;
	float m_damping = 0.4f;

	float m_timeStep = 0.03f;
	float m_spatialStep = 0.8f;

	const UINT m_numRows = 200;
	const UINT m_numCols = 200;

};