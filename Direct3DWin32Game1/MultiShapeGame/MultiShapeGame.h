#pragma once
#include "Common/RenderObject.h"
#include "MultiObjectGame/MultiObjectGame.h"

#include "BoxGame/BoxGame.h"

// struct BoxGameVertex
// {
// 	DirectX::XMFLOAT3 position;
// 	DirectX::XMFLOAT4 color;
// };

class MultiShapeGame : public MultiObjectGame
{
	using Super = MultiObjectGame;

public:

	virtual ~MultiShapeGame();

	virtual void Initialize(HWND window, int width, int height) override;

protected:

	virtual void AddObjects() override;
};

class Box : public RenderObject
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
	virtual void BuildShape();
};

class Box2 : public Box
{
public:
	using Super = Box;

	virtual void Update(DX::StepTimer const& timer);

protected:
	virtual void BuildShape();
};

class Pyramid : public Box
{
public:
	using Super = Box;

	virtual void Update(DX::StepTimer const& timer);

protected:
	virtual void BuildShape();
};