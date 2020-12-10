#include "pch.h"
#include "MultiObjectGame/MultiObjectGame.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

void MultiObjectGame::Initialize(HWND window, int width, int height)
{

	Super::Initialize(window, width, height);

	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		(*it)->Initialize(m_d3dDevice,m_d3dContext,&m_view,&m_proj);
	}
}

void MultiObjectGame::Tick()
{
	Super::Update(m_timer);

	m_timer.Tick([&]()
	{
		// Don't try to render anything before the first Update.
		bool bDraw = m_timer.GetFrameCount() != 0;

		if (bDraw)
		{
			Clear();
		}

		for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
		{
			(*it)->Update(m_timer);

			if (bDraw)
			{
				(*it)->Render();
			}
		}	
		
		if (bDraw)
		{
			Present();
		}
	});
}