#include "pch.h"
#include "ShadowGame/ShadowGame.h"
#include "Common/GeometryGenerator.h"
#include "ScreenGrab.h"

#include "MirrorGame/MirrorGame.h"

using namespace DirectX;
using Microsoft::WRL::ComPtr;

ShadowGame::~ShadowGame()
{
	delete m_screenQuad;
	m_screenQuad = nullptr;
}

void ShadowGame::Initialize(HWND window, int width, int height)
{
	Super::Initialize(window, width, height);

	BuildShadowMapViews();

	CD3D11_SAMPLER_DESC samDesc(D3D11_DEFAULT);
	samDesc.Filter = D3D11_FILTER_COMPARISON_MIN_MAG_LINEAR_MIP_POINT;
	samDesc.AddressU = D3D11_TEXTURE_ADDRESS_BORDER;
	samDesc.AddressV = D3D11_TEXTURE_ADDRESS_BORDER;
	samDesc.AddressW = D3D11_TEXTURE_ADDRESS_BORDER;
	samDesc.ComparisonFunc = D3D11_COMPARISON_LESS_EQUAL;
	float outsideShadowFactor = 1.f;
	samDesc.BorderColor[0] = outsideShadowFactor;
	samDesc.BorderColor[1] = outsideShadowFactor;
	samDesc.BorderColor[2] = outsideShadowFactor;
	samDesc.BorderColor[3] = outsideShadowFactor;

	ComPtr<ID3D11SamplerState> sam;
	HRESULT hr = m_d3dDevice->CreateSamplerState(&samDesc,sam.GetAddressOf());
	DX::ThrowIfFailed(hr);
	m_d3dContext->PSSetSamplers(1, 1, sam.GetAddressOf());

	if (m_screenQuad)
		m_screenQuad->Initialize(m_d3dDevice, m_d3dContext, &m_view, &m_proj);

	m_spotLight.Position = XMFLOAT3(0.0f, 10.0f, 0.0f);
}

void ShadowGame::OnKeyButtonReleased(WPARAM key)
{
	Super::OnKeyButtonReleased(key);

	// Toggle using spot light or directional light
	if (key == 'L')
	{
		bUsingSpotOrDirectionalLight = !bUsingSpotOrDirectionalLight;
	}
}

void ShadowGame::AddObjects()
{
	//Super::AddObjects();
	
	m_objects.push_back(new Floor());
	m_objects.push_back(new LitCrate());

	m_screenQuad = new ShadowMapQuad();
}

void ShadowGame::PreObjectsRender()
{
	Super::PreObjectsRender();

	RenderShadowMapTexture();
}

void ShadowGame::PostObjectsRender()
{
	if (m_screenQuad)
	{
		m_screenQuad->Update(m_timer);
		m_screenQuad->Render();
	}
}

void ShadowGame::BuildConstantBuffer()
{
	CreateConstantBufferPerFrame(sizeof(cbPerFrame));
}

void ShadowGame::UpdateConstantBufferPerFrame()
{
	m_cbPerFrame.dirLight = m_dirLight;
	m_cbPerFrame.pointLight = m_pointLight;
	m_cbPerFrame.spotLight = m_spotLight;
	m_cbPerFrame.eyePosW = m_eyePos;
	m_cbPerFrame.fogColor = XMFLOAT4(Colors::Silver);
	m_cbPerFrame.fogStart = 15.f;
	m_cbPerFrame.fogRange = 175.f;

	XMMATRIX view = XMLoadFloat4x4(&m_view);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);
	XMMATRIX viewProj = XMMatrixMultiply(view, proj);
	XMStoreFloat4x4(&m_cbPerFrame.viewProj, XMMatrixTranspose(viewProj));

	d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerFrame, m_cbPerFrame);
}

void ShadowGame::BuildShadowMapViews()
{
	// Use typeless format because the DSV is going to interpret
	// the bits as DXGI_FORMAT_D24_UNORM_S8_UINT, whereas the SRV is going to interpret
	// the bits as DXGI_FORMAT_R24_UNORM_X8_TYPELESS.

	// Create Texture

	D3D11_TEXTURE2D_DESC depthStencilDesc;
	depthStencilDesc.Width = m_outputWidth;
	depthStencilDesc.Height = m_outputHeight;
	depthStencilDesc.MipLevels = 1;
	depthStencilDesc.ArraySize = 1;
	depthStencilDesc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depthStencilDesc.SampleDesc.Count = 1;
	depthStencilDesc.SampleDesc.Quality = 0;
	depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
	depthStencilDesc.BindFlags = D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE;
	depthStencilDesc.CPUAccessFlags = 0;
	depthStencilDesc.MiscFlags = 0;

	HRESULT hr = m_d3dDevice->CreateTexture2D(&depthStencilDesc, nullptr, m_shadowMapTexture.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Create Depth Stencil View

	CD3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D24_UNORM_S8_UINT);
	hr = m_d3dDevice->CreateDepthStencilView(m_shadowMapTexture.Get(), &dsvDesc, m_shadowMapDSV.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Create Shader Resource View

	CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D,DXGI_FORMAT_R24_UNORM_X8_TYPELESS);
	hr = m_d3dDevice->CreateShaderResourceView(m_shadowMapTexture.Get(), &srvDesc, m_shadowMapSRV.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Build the shader that use shadow

	D3D_SHADER_MACRO defines[] =
	{
		"ENABLE_SHADOWMAP", "1",
		NULL, NULL
	};
	const std::wstring shaderFilename = L"ShadowGame\\Shadow.hlsl";

	Microsoft::WRL::ComPtr<ID3DBlob> mVSByteCode = d3dUtil::CompileShader(shaderFilename, defines, "VS", "vs_5_0");
	Microsoft::WRL::ComPtr<ID3DBlob> mPSByteCode = d3dUtil::CompileShader(shaderFilename, defines, "PS", "ps_5_0");

	hr = m_d3dDevice->CreateVertexShader(mVSByteCode->GetBufferPointer(), mVSByteCode->GetBufferSize(), nullptr, m_shadowMapVertexShader.GetAddressOf());
	DX::ThrowIfFailed(hr);
	hr = m_d3dDevice->CreatePixelShader(mPSByteCode->GetBufferPointer(), mPSByteCode->GetBufferSize(), nullptr, m_shadowMapPixelShader.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Set the shader to draw the shadow
	if (m_cylinder)
	{
		m_cylinder->SetVSAndPSShader(m_shadowMapVertexShader, m_shadowMapPixelShader);
	}
	for (int i = 0; i < m_objects.size(); ++i)
	{
		auto* shapeObject = dynamic_cast<LitShape*>(m_objects[i]);
		auto* normalObject = dynamic_cast<NormalMapShape*>(m_objects[i]);
		auto* skyObject = dynamic_cast<SkyBox*>(m_objects[i]);
		if (shapeObject && !normalObject && !skyObject)
		{
			shapeObject->SetVSAndPSShader(m_shadowMapVertexShader, m_shadowMapPixelShader);
		}
	}
}

void ShadowGame::RenderShadowMapTexture()
{
// This macro will render the colorful scene form light to a texture and show in ScreenQuad. Otherwise the ScreenQuad will show the depth buffer.
//#define TEST_TEXTURE

	ID3D11ShaderResourceView* nullSRV[] = { nullptr };
	m_d3dContext->PSSetShaderResources(3, 1, nullSRV);

	HRESULT hr;

#ifdef TEST_TEXTURE
	 //Test: Render the scene to texture
	 //Create Texture
	CD3D11_TEXTURE2D_DESC texDesc(DXGI_FORMAT_R8G8B8A8_UNORM, m_outputWidth, m_outputHeight,1,1);
	texDesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
	ComPtr<ID3D11Texture2D> mTex;
	hr = m_d3dDevice->CreateTexture2D(&texDesc, nullptr, mTex.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Create Shader Resource View
	ComPtr<ID3D11ShaderResourceView> mSRV;
	CD3D11_SHADER_RESOURCE_VIEW_DESC srvDesc(D3D11_SRV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM);
	hr = m_d3dDevice->CreateShaderResourceView(mTex.Get(), &srvDesc, mSRV.GetAddressOf());
	DX::ThrowIfFailed(hr);

	// Create Render Target View
	ComPtr<ID3D11RenderTargetView> mRTV;
	CD3D11_RENDER_TARGET_VIEW_DESC rtvDesc(D3D11_RTV_DIMENSION_TEXTURE2D, DXGI_FORMAT_R8G8B8A8_UNORM);
	hr = m_d3dDevice->CreateRenderTargetView(mTex.Get(), &rtvDesc, mRTV.GetAddressOf());
	DX::ThrowIfFailed(hr);
	 
	m_d3dContext->ClearRenderTargetView(mRTV.Get(), Colors::CornflowerBlue);
	m_d3dContext->ClearDepthStencilView(m_shadowMapDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	m_d3dContext->OMSetRenderTargets(1, mRTV.GetAddressOf(), m_shadowMapDSV.Get());
#else
	m_d3dContext->ClearDepthStencilView(m_shadowMapDSV.Get(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0);
	ID3D11RenderTargetView* nullTarget[] = { nullptr };
	m_d3dContext->OMSetRenderTargets(1, nullTarget, m_shadowMapDSV.Get());
#endif

	XMMATRIX originalView = XMLoadFloat4x4(&m_view);
	XMMATRIX originalProj = XMLoadFloat4x4(&m_proj);

	// Render the scene from light

	XMFLOAT3 pos;
	XMFLOAT3 dir;

	if (bUsingSpotOrDirectionalLight)
	{
		pos = m_spotLight.Position;
		dir = m_spotLight.Direction;
	}
	else if(bUsingDirectionalorPointLight)
	{
		dir = m_dirLight.Direction;
		float distance = 200.f;
		pos = XMFLOAT3(-dir.x*distance, -dir.y*distance, -dir.z*distance);
	}
	else
	{
		pos = m_pointLight.Position;
		dir = XMFLOAT3(0.f, -1.f, 0.f);
	}

	XMVECTOR up = XMVectorSet(0.f, 1.f, 0.f, 0.f);
	if (dir.y == -1.f || dir.y == 1.f)
	{
		up = XMVectorSet(1.f, 0.f, 0.f, 0.f);
	}
	XMMATRIX view = XMMatrixLookToLH(XMLoadFloat3(&pos), XMLoadFloat3(&dir), up);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);
	// The depth is nearly 0.9-1 if using the default value of near/far Z.
	// Following test code can make you observe the depth map
	//XMMATRIX proj = XMMatrixPerspectiveFovLH(m_fovAngleY, AspectRatio(), 50.f, 300.f);
	//XMMATRIX proj = XMMatrixPerspectiveFovLH(m_fovAngleY, AspectRatio(), 5.f, 50.f);

	if (!bUsingSpotOrDirectionalLight && bUsingDirectionalorPointLight)
	{
		float width = 20.f;
		float height = 20.f;
		// For large scene. you need ensure it contain all objects in scene
		//float width = 100.f;
		//float height = 100.f;
		proj = XMMatrixOrthographicLH(width, height, 1.f, 1000.f);
	}
	
	XMStoreFloat4x4(&m_view, view);
	XMStoreFloat4x4(&m_proj, proj);
	XMStoreFloat4x4(&m_cbPerFrame.viewProj, XMMatrixTranspose(XMMatrixMultiply(view, proj)));
	// Transform NDC space [-1,+1]^2 to texture space [0,1]^2
	// It's the same as following
	//shadowPosH.x = +0.5f*shadowPosH.x + 0.5f;
	//shadowPosH.y = -0.5f*shadowPosH.y + 0.5f;
	XMMATRIX T(
		0.5f, 0.0f, 0.0f, 0.0f,
		0.0f, -0.5f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.5f, 0.5f, 0.0f, 1.0f);
	XMStoreFloat4x4(&m_cbPerFrame.shadowTransform, XMMatrixTranspose(XMMatrixMultiply(XMMatrixMultiply(view, proj),T)));
	d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerFrame, m_cbPerFrame);

	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		Shape* shapeObject = dynamic_cast<Shape*>(*it);
		if (shapeObject)
		{
			shapeObject->UpdateConstantBufferPerObject();
		}
	}

	ComPtr<ID3D11RasterizerState> prevRSState;
	m_d3dContext->RSGetState(prevRSState.GetAddressOf());
	// [From MSDN]
	// If the depth buffer currently bound to the output-merger stage has a UNORM format or
	// no depth buffer is bound the bias value is calculated like this: 
	//
	// Bias = (float)DepthBias * r + SlopeScaledDepthBias * MaxDepthSlope;
	//
	// where r is the minimum representable value > 0 in the depth-buffer format converted to float32.
	// [/End MSDN]
	// 
	// For a 24-bit depth buffer, r = 1 / 2^24.
	//
	// Example: DepthBias = 100000 ==> Actual DepthBias = 100000/2^24 = .006
	CD3D11_RASTERIZER_DESC rsDesc(D3D11_DEFAULT);
	rsDesc.DepthBias = 1000;
	rsDesc.DepthBiasClamp = 0.f;
	rsDesc.SlopeScaledDepthBias = 1.f;
	ComPtr<ID3D11RasterizerState> mRSState;
	hr = m_d3dDevice->CreateRasterizerState(&rsDesc, mRSState.GetAddressOf());
	DX::ThrowIfFailed(hr);
	m_d3dContext->RSSetState(mRSState.Get());

	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		(*it)->Render();
	}

	m_d3dContext->RSSetState(prevRSState.Get());

	m_d3dContext->OMSetRenderTargets(1, m_renderTargetView.GetAddressOf(), m_depthStencilView.Get());

	XMStoreFloat4x4(&m_view, originalView);
	XMStoreFloat4x4(&m_proj, originalProj);
	XMStoreFloat4x4(&m_cbPerFrame.viewProj, XMMatrixTranspose(XMMatrixMultiply(originalView, originalProj)));
	d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerFrame, m_cbPerFrame);

	for (auto it = m_objects.begin(); it != m_objects.end(); ++it)
	{
		Shape* shapeObject = dynamic_cast<Shape*>(*it);
		if (shapeObject)
		{
			shapeObject->UpdateConstantBufferPerObject();
		}
	}

	if (m_screenQuad)
	{
#ifdef TEST_TEXTURE
		m_screenQuad->UpdateTexture(mSRV);
#else
		m_screenQuad->UpdateTexture(m_shadowMapSRV);
#endif
	}

	m_d3dContext->PSSetShaderResources(3, 1, m_shadowMapSRV.GetAddressOf());
}

void ScreenQuad::UpdateConstantBufferPerObject()
{
	XMStoreFloat4x4(&m_cbPerObject.worldViewProj, XMMatrixTranspose(m_transformMatrix));

	d3dUtil::UpdateDynamicBufferFromData(m_d3dContext, m_constantBufferPerObject, m_cbPerObject);
}

void ScreenQuad::UpdateTexture(Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>& inSRV)
{
	m_diffuseMapView = inSRV;
}

void ScreenQuad::BuildShader()
{
	const std::wstring shaderFilename = L"CrateGame\\Crate.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename);
}

void ScreenQuad::BuildTexture()
{
	BuildTextureByName(L"CrateGame\\WoodCrate01.dds", m_diffuseMapView);
}

void ScreenQuad::BuildShape()
{
	using VertexType = VertexPositionNormalUV;

	GeometryGenerator::MeshData quad;

	GeometryGenerator geoGen;
	geoGen.CreateFullscreenQuad(quad);

	//
	// Extract the vertex elements we are interested in and pack the
	// vertices of all the meshes into one vertex buffer.
	//

	std::vector<VertexType> vertices(quad.Vertices.size());

	for (UINT i = 0; i < quad.Vertices.size(); ++i)
	{
		vertices[i].position = quad.Vertices[i].Position;
		vertices[i].normal = quad.Vertices[i].Normal;
		vertices[i].textureUV = quad.Vertices[i].TexC;
	}

	D3D11_BUFFER_DESC vbDesc;
	vbDesc.ByteWidth = sizeof(VertexType) * vertices.size();
	vbDesc.Usage = D3D11_USAGE_IMMUTABLE;
	vbDesc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	vbDesc.CPUAccessFlags = 0;
	vbDesc.MiscFlags = 0;
	vbDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA vbInitData;
	vbInitData.pSysMem = vertices.data();
	vbInitData.SysMemPitch = 0;
	vbInitData.SysMemSlicePitch = 0;

	HRESULT hr = m_d3dDevice->CreateBuffer(&vbDesc, &vbInitData, m_vertexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);

	m_indexCount = quad.Indices.size();

	D3D11_BUFFER_DESC ibDesc;
	ibDesc.ByteWidth = sizeof(UINT) * quad.Indices.size();
	ibDesc.Usage = D3D11_USAGE_IMMUTABLE;
	ibDesc.BindFlags = D3D11_BIND_INDEX_BUFFER;
	ibDesc.CPUAccessFlags = 0;
	ibDesc.MiscFlags = 0;
	ibDesc.StructureByteStride = 0;

	D3D11_SUBRESOURCE_DATA ibInitData;
	ibInitData.pSysMem = quad.Indices.data();
	ibInitData.SysMemPitch = 0;
	ibInitData.SysMemSlicePitch = 0;

	hr = m_d3dDevice->CreateBuffer(&ibDesc, &ibInitData, m_indexBuffer.GetAddressOf());
	DX::ThrowIfFailed(hr);
}

void ScreenQuad::BuildConstantBuffer()
{
	CreateConstantBufferPerObject(sizeof(cbPerObjectStruct));
}

ShadowMapQuad::ShadowMapQuad()
{
	float scaleX = 0.3f;
	float scaleY = 0.3f;
	float offsetX = 1.f - scaleX;
	float offsetY = 1.f - scaleY;
	XMMATRIX transformMatrix = XMMatrixMultiply(XMMatrixScaling(scaleX, scaleY, 1.f), XMMatrixTranslation(offsetX, -offsetY, 0.f));
	SetTransformMatrix(transformMatrix);
}

void ShadowMapQuad::BuildShader()
{
	const std::wstring shaderFilename = L"ShadowGame\\ShadowMapQuad.hlsl";
	CreateVSAndPSShader(shaderFilename, shaderFilename);
}