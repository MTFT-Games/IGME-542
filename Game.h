#pragma once

#include "DXCore.h"
#include <DirectXMath.h>
#include <wrl/client.h> // Used for ComPtr - a smart pointer for COM objects
#include "Camera.h"
#include <memory>
#include <vector>
#include "Mesh.h"
#include "Renderable.h"
#include "DX12Helper.h"
#include "Lights.h"

class Game 
	: public DXCore
{

public:
	Game(HINSTANCE hInstance);
	~Game();

	// Overridden setup and game loop methods, which
	// will be called automatically
	void Init();
	void OnResize();
	void Update(float deltaTime, float totalTime);
	void Draw(float deltaTime, float totalTime);

private:

	// Initialization helper methods - feel free to customize, combine, remove, etc.
	void CreateRootSigAndPipelineState();
	void CreateBasicGeometry();

	// Note the usage of ComPtr below
	//  - This is a smart pointer for objects that abide by the
	//     Component Object Model, which DirectX objects do
	//  - More info here: https://github.com/Microsoft/DirectXTK/wiki/ComPtr

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexBuffer;

	D3D12_VERTEX_BUFFER_VIEW vbView;
	D3D12_INDEX_BUFFER_VIEW ibView;

	std::shared_ptr<Camera> camera;

	std::vector<std::shared_ptr<Mesh>> meshList;
	std::vector<Renderable> renderableList;

	Light lights[20];
	int lightCount;

	DX12Helper& dx12Helper;
};

