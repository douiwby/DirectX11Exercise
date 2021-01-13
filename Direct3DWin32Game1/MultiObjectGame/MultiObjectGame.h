#pragma once
#include "InitGame/InitGame.h"
#include "Common/d3dUtil.h"
#include "Common/RenderObject.h"
#include <vector>

class MultiObjectGame : public InitGame
{
	using Super = InitGame;
public:

	virtual ~MultiObjectGame() = default;

	virtual void Initialize(HWND window, int width, int height);
	virtual void Tick();

protected:

	virtual void Update(DX::StepTimer const& timer);

	std::vector<RenderObject*> m_objects;
};