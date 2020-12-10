#include "pch.h"
#include "MultiObjectGame/RenderObject.h"

using namespace DirectX;

RenderObject::~RenderObject()
{
	delete m_world;
}

void RenderObject::Initialize(
	Microsoft::WRL::ComPtr<ID3D11Device>& device,
	Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
	DirectX::XMFLOAT4X4* view,
	DirectX::XMFLOAT4X4* proj)
{
	m_d3dDevice = device;
	m_d3dContext = context;
	m_view = view;
	m_proj = proj;

	m_world = new XMFLOAT4X4();
	XMStoreFloat4x4(m_world, XMMatrixIdentity());
}