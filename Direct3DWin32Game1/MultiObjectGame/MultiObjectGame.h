#pragma once
#include "InitGame/InitGame.h"
#include "Common/d3dUtil.h"
#include "MultiObjectGame/RenderObject.h"
#include <vector>

class MultiObjectGame : public InitGame
{
public:
	using Super = InitGame;

	virtual ~MultiObjectGame() = default;

	virtual void Initialize(HWND window, int width, int height);
	virtual void Tick();

protected:
	std::vector<RenderObject*> m_objects;
};