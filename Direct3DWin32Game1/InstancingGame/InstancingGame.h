#pragma once
#include "MultiObjectGame/MultiObjectGame.h"
#include "TransparentWaveGame/TransparentWaveGame.h"

namespace DirectX
{
	class BoundingBox;
}

using VertexType = VertexPositionNormalUV;

struct InstanceData
{
	DirectX::XMFLOAT4X4 World;
	UINT     MaterialIndex;
	UINT     InstPad0;
	UINT     InstPad1;
	UINT     InstPad2;
};

class InstancingGame : public LitHillGame
{
	using Super = LitHillGame;

public:

	InstancingGame()
	{
		m_maxRadius = 500;
	}

	virtual void Initialize(HWND window, int width, int height) override;

	virtual void OnKeyButtonPressed(WPARAM key) override;

	void ToggleFrustumCulling();

	virtual void OnMouseDown(WPARAM btnState, int x, int y) override;

protected:

	virtual void Update(DX::StepTimer const& timer) override;

	virtual void AddObjects() override;

	virtual void PostObjectsRender() override;

	virtual void BuildConstantBuffer() override;

	virtual void CalculateFrameStats() override;

	void BuildInstancedBuffer();

	void Pick(int x, int y);

	struct cbPerFrame
	{
		DirectionalLight dirLight;
		PointLight pointLight;
		SpotLight spotLight;
		DirectX::XMFLOAT4 eyePosW;
		DirectX::XMFLOAT4 fogColor;
		float fogStart;
		float fogRange;
		DirectX::XMFLOAT2 cbPerFramePad;
		DirectX::XMFLOAT4X4 viewProj;
	} m_cbPerFrame;	

	Microsoft::WRL::ComPtr<ID3D11Buffer> m_instanceDataBuffer;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_instanceDataSRV;

	std::vector<InstanceData> m_instancedDataArray;
	std::vector<InstanceData> m_cullingDataArray;

	class InstancingCrate* m_instanceCrate;
	int m_instanceCount = 0;
	bool bFrustumCullingEnable = true;

	class PickedTriangle* m_pickedTriangle;
	UINT m_pickedInstaceIndex = UINT_MAX;
	UINT m_pickedTriangleIndex = UINT_MAX;
	bool bPickSuccess = false;
	bool bNewPicked = false;
};

class InstancingCrate : public LitShape
{
	using Super = LitShape;

	friend class InstancingGame;

public:

	InstancingCrate();
	~InstancingCrate();

	virtual void Initialize(
		Microsoft::WRL::ComPtr<ID3D11Device>& device,
		Microsoft::WRL::ComPtr<ID3D11DeviceContext>& context,
		DirectX::XMFLOAT4X4* view,
		DirectX::XMFLOAT4X4* proj) override;

	virtual void Update(DX::StepTimer const& timer) override;
	virtual void Render() override;

protected:

	virtual void BuildTexture() override;
	virtual void BuildShader() override;
	virtual void BuildShape() override;
	virtual void BuildMaterial() override;
	virtual void BuildConstantBuffer() override;

	int m_texArraySize = 0;
	int m_matCount = 4;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> m_matDataSRV;	
	DirectX::BoundingBox * m_bounds;
	std::vector<VertexType> m_vertices;
	std::vector<UINT> m_indices;
};

class PickedTriangle : Shape
{
	using Super = Shape;

	friend class InstancingGame;

public:

	virtual void Render() override;

protected:

	virtual void BuildShape() override;
	virtual void BuildConstantBuffer() override;
};