#include "Game.h"
#include "Vertex.h"
#include "Input.h"
#include "PathHelpers.h"
#include "BufferStructs.h"
#include "WICTextureLoader.h"
#include "RaytracingHelper.h"

// Needed for a helper function to load pre-compiled shader files
#pragma comment(lib, "d3dcompiler.lib")
#include <d3dcompiler.h>

// For the DirectX Math library
using namespace DirectX;

// --------------------------------------------------------
// Constructor
//
// DXCore (base class) constructor will set up underlying fields.
// Direct3D itself, and our window, are not ready at this point!
//
// hInstance - the application's OS-level handle (unique ID)
// --------------------------------------------------------
Game::Game(HINSTANCE hInstance)
	: DXCore(
		hInstance,			// The application's handle
		L"DirectX Game",	// Text for the window's title bar (as a wide-character string)
		1280,				// Width of the window's client area
		720,				// Height of the window's client area
		false,				// Sync the framerate to the monitor refresh? (lock framerate)
		true),  			// Show extra stats (fps) in title bar?
	ibView({}),
	vbView({}),
	lights(),
	lightCount(0),
	dx12Helper(DX12Helper::GetInstance())
{
#if defined(DEBUG) || defined(_DEBUG)
	// Do we want a console window?  Probably only in debug mode
	CreateConsoleWindow(500, 120, 32, 120);
	printf("Console window created successfully.  Feel free to printf() here.\n");
#endif
}

// --------------------------------------------------------
// Destructor - Clean up anything our game has created:
//  - Delete all objects manually created within this class
//  - Release() all Direct3D objects created within this class
// --------------------------------------------------------
Game::~Game()
{
	// Call delete or delete[] on any objects or arrays you've
	// created using new or new[] within this class
	// - Note: this is unnecessary if using smart pointers

	// Call Release() on any Direct3D objects made within this class
	// - Note: this is unnecessary for D3D objects stored in ComPtrs

	// We need to wait here until the GPU
	// is actually done with its work
	dx12Helper.WaitForGPU();
	delete& RaytracingHelper::GetInstance();
}

// --------------------------------------------------------
// Called once per program, after Direct3D and the window
// are initialized but before the game loop.
// --------------------------------------------------------
void Game::Init()
{
	// Attempt to initialize DXR
	RaytracingHelper::GetInstance().Initialize(
		windowWidth,
		windowHeight,
		device,
		commandQueue,
		commandList,
		FixPath(L"Raytracing.cso"));

	// Helper methods for loading shaders, creating some basic
	// geometry to draw and some simple camera matrices.
	// - You'll be expanding and/or replacing these later
	CreateRootSigAndPipelineState();
	CreateBasicGeometry();
	camera = std::make_shared<Camera>(
		windowWidth / (float)windowHeight, 
		XMFLOAT3(0, 0, -3), 
		XMFLOAT3(), 
		3.14f / 2);
	
	lights[0].Type = LIGHT_TYPE_DIRECTIONAL;
	lights[0].Direction = XMFLOAT3(0, -0.8f, 0.2f);
	lights[0].Color = XMFLOAT3(1.0f, 1.0f, 1.0f);
	lights[0].intensity = 1.0f;

	lights[1].Type = LIGHT_TYPE_DIRECTIONAL;
	lights[1].Direction = XMFLOAT3(-1, 0, 0);
	lights[1].Color = XMFLOAT3(0.76f, 0.76f, 0.76f);
	lights[1].intensity = 0.4f;

	lights[2].Type = LIGHT_TYPE_DIRECTIONAL;
	lights[2].Direction = XMFLOAT3(1, 0, 0);
	lights[2].Color = XMFLOAT3(0.36f, 0.36f, 0.36f);
	lights[2].intensity = 0.7f;

	lights[3].Type = LIGHT_TYPE_POINT;
	lights[3].Position = XMFLOAT3(0, -3, 0);
	lights[3].Color = XMFLOAT3(0, 0.7f, 0.8f);
	lights[3].intensity = 1.0f;
	lights[3].Range = 5;

	lights[4].Type = LIGHT_TYPE_POINT;
	lights[4].Position = XMFLOAT3(-1, 0, 1);
	lights[4].Color = XMFLOAT3(0.8f, 0.1f, 0.2f);
	lights[4].intensity = 0.4f;
	lights[4].Range = 5;

	lightCount = 5;
}

// --------------------------------------------------------
// Loads the two basic shaders, then creates the root signature
// and pipeline state object for our very basic demo.
// --------------------------------------------------------
void Game::CreateRootSigAndPipelineState()
{
	// Blobs to hold raw shader byte code used in several steps below
	Microsoft::WRL::ComPtr<ID3DBlob> vertexShaderByteCode;
	Microsoft::WRL::ComPtr<ID3DBlob> pixelShaderByteCode;

	// Load shaders
	{
		// Read our compiled vertex shader code into a blob
		// - Essentially just "open the file and plop its contents here"
		D3DReadFileToBlob(FixPath(L"VertexShader.cso").c_str(), vertexShaderByteCode.GetAddressOf());
		D3DReadFileToBlob(FixPath(L"PixelShader.cso").c_str(), pixelShaderByteCode.GetAddressOf());
	}

	// Input layout
	const unsigned int inputElementCount = 4;
	D3D12_INPUT_ELEMENT_DESC inputElements[inputElementCount] = {};
	{
		// Create an input layout that describes the vertex format
		// used by the vertex shader we're using
		// - This is used by the pipeline to know how to interpret the raw data
		// sitting inside a vertex buffer
		// 
		// Set up the first element - a position, which is 3 float values
		inputElements[0].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[0].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[0].SemanticName = "POSITION";
		inputElements[0].SemanticIndex = 0;

		inputElements[1].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[1].Format = DXGI_FORMAT_R32G32_FLOAT;
		inputElements[1].SemanticName = "TEXCOORD";
		inputElements[1].SemanticIndex = 0;

		inputElements[2].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[2].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[2].SemanticName = "NORMAL";
		inputElements[2].SemanticIndex = 0;

		inputElements[3].AlignedByteOffset = D3D12_APPEND_ALIGNED_ELEMENT;
		inputElements[3].Format = DXGI_FORMAT_R32G32B32_FLOAT;
		inputElements[3].SemanticName = "TANGENT";
		inputElements[3].SemanticIndex = 0;
	}

	// Root Signature
	{
		// Describe the range of CBVs needed for the vertex shader
		D3D12_DESCRIPTOR_RANGE cbvRangeVS = {};
		cbvRangeVS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangeVS.NumDescriptors = 1;
		cbvRangeVS.BaseShaderRegister = 0;
		cbvRangeVS.RegisterSpace = 0;
		cbvRangeVS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		
		// Describe the range of CBVs needed for the pixel shader
		D3D12_DESCRIPTOR_RANGE cbvRangePS = {};
		cbvRangePS.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
		cbvRangePS.NumDescriptors = 1;
		cbvRangePS.BaseShaderRegister = 0;
		cbvRangePS.RegisterSpace = 0;
		cbvRangePS.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		
		// Create a range of SRV's for textures
		D3D12_DESCRIPTOR_RANGE srvRange = {};
		srvRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
		srvRange.NumDescriptors = 4; // Set to max number of textures at once (match pixel shader!)
		srvRange.BaseShaderRegister = 0; // Starts at s0 (match pixel shader!)
		srvRange.RegisterSpace = 0;
		srvRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;
		
		// Create the root parameters
		D3D12_ROOT_PARAMETER rootParams[3] = {};
		
		// CBV table param for vertex shader
		rootParams[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;
		rootParams[0].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[0].DescriptorTable.pDescriptorRanges = &cbvRangeVS;
		
		// CBV table param for pixel shader
		rootParams[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[1].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[1].DescriptorTable.pDescriptorRanges = &cbvRangePS;
		
		// SRV table param
		rootParams[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
		rootParams[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		rootParams[2].DescriptorTable.NumDescriptorRanges = 1;
		rootParams[2].DescriptorTable.pDescriptorRanges = &srvRange;
		
		// Create a single static sampler (available to all pixel shaders at the same slot)
		D3D12_STATIC_SAMPLER_DESC anisoWrap = {};
		anisoWrap.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
		anisoWrap.Filter = D3D12_FILTER_ANISOTROPIC;
		anisoWrap.MaxAnisotropy = 16;
		anisoWrap.MaxLOD = D3D12_FLOAT32_MAX;
		anisoWrap.ShaderRegister = 0; // register(s0)
		anisoWrap.ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
		D3D12_STATIC_SAMPLER_DESC samplers[] = { anisoWrap };
		
		// Describe and serialize the root signature
		D3D12_ROOT_SIGNATURE_DESC rootSig = {};
		rootSig.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;
		rootSig.NumParameters = ARRAYSIZE(rootParams);
		rootSig.pParameters = rootParams;
		rootSig.NumStaticSamplers = ARRAYSIZE(samplers);
		rootSig.pStaticSamplers = samplers;

		ID3DBlob* serializedRootSig = 0;
		ID3DBlob* errors = 0;

		D3D12SerializeRootSignature(
			&rootSig,
			D3D_ROOT_SIGNATURE_VERSION_1,
			&serializedRootSig,
			&errors);

		// Check for errors during serialization
		if (errors != 0)
		{
			OutputDebugString((wchar_t*)errors->GetBufferPointer());
		}

		// Actually create the root sig
		device->CreateRootSignature(
			0,
			serializedRootSig->GetBufferPointer(),
			serializedRootSig->GetBufferSize(),
			IID_PPV_ARGS(rootSignature.GetAddressOf()));
	}

	// Pipeline state
	{
		// Describe the pipeline state
		D3D12_GRAPHICS_PIPELINE_STATE_DESC psoDesc = {};

		// -- Input assembler related ---
		psoDesc.InputLayout.NumElements = inputElementCount;
		psoDesc.InputLayout.pInputElementDescs = inputElements;
		psoDesc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;

		// Root sig
		psoDesc.pRootSignature = rootSignature.Get();

		// -- Shaders (VS/PS) ---
		psoDesc.VS.pShaderBytecode = vertexShaderByteCode->GetBufferPointer();
		psoDesc.VS.BytecodeLength = vertexShaderByteCode->GetBufferSize();
		psoDesc.PS.pShaderBytecode = pixelShaderByteCode->GetBufferPointer();
		psoDesc.PS.BytecodeLength = pixelShaderByteCode->GetBufferSize();

		// -- Render targets ---
		psoDesc.NumRenderTargets = 1;
		psoDesc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
		psoDesc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
		psoDesc.SampleDesc.Count = 1;
		psoDesc.SampleDesc.Quality = 0;

		// -- States ---
		psoDesc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
		psoDesc.RasterizerState.CullMode = D3D12_CULL_MODE_BACK;
		psoDesc.RasterizerState.DepthClipEnable = true;

		psoDesc.DepthStencilState.DepthEnable = true;
		psoDesc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
		psoDesc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;

		psoDesc.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
		psoDesc.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
		psoDesc.BlendState.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
		psoDesc.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;

		// -- Misc ---
		psoDesc.SampleMask = 0xffffffff;

		// Create the pipe state object
		device->CreateGraphicsPipelineState(&psoDesc,
			IID_PPV_ARGS(pipelineState.GetAddressOf()));
	}
}

// --------------------------------------------------------
// Creates the geometry we're going to draw - a single triangle for now
// --------------------------------------------------------
void Game::CreateBasicGeometry()
{
	// Create some temporary variables to represent colors
	// - Not necessary, just makes things more readable
	XMFLOAT4 red	= XMFLOAT4(1.0f, 0.0f, 0.0f, 1.0f);
	XMFLOAT4 green	= XMFLOAT4(0.0f, 1.0f, 0.0f, 1.0f);
	XMFLOAT4 blue	= XMFLOAT4(0.0f, 0.0f, 1.0f, 1.0f);

	// Set up the vertices of the triangle we would like to draw
	// - We're going to copy this array, exactly as it exists in CPU memory
	//    over to a Direct3D-controlled data structure on the GPU (the vertex buffer)
	// - Note: Since we don't have a camera or really any concept of
	//    a "3d world" yet, we're simply describing positions within the
	//    bounds of how the rasterizer sees our screen: [-1 to +1] on X and Y
	// - This means (0,0) is at the very center of the screen.
	// - These are known as "Normalized Device Coordinates" or "Homogeneous 
	//    Screen Coords", which are ways to describe a position without
	//    knowing the exact size (in pixels) of the image/window/etc.  
	// - Long story short: Resizing the window also resizes the triangle,
	//    since we're describing the triangle in terms of the window itself
	/*Vertex vertices[] =
	{
		{ XMFLOAT3(+0.0f, +0.5f, +0.0f), red },
		{ XMFLOAT3(+0.5f, -0.5f, +0.0f), blue },
		{ XMFLOAT3(-0.5f, -0.5f, +0.0f), green },
	};

	// Set up indices, which tell us which vertices to use and in which order
	// - This is redundant for just 3 vertices, but will be more useful later
	// - Indices are technically not required if the vertices are in the buffer 
	//    in the correct order and each one will be used exactly once
	// - But just to see how it's done...
	unsigned int indices[] = { 0, 1, 2 };


	// Create the two buffers
	DX12Helper& dx12Helper = DX12Helper::GetInstance();
	vertexBuffer = dx12Helper.CreateStaticBuffer(sizeof(Vertex), ARRAYSIZE(vertices), vertices);
	indexBuffer = dx12Helper.CreateStaticBuffer(sizeof(unsigned int), ARRAYSIZE(indices), indices);

	// Set up the views
	vbView.StrideInBytes = sizeof(Vertex);
	vbView.SizeInBytes = sizeof(Vertex) * ARRAYSIZE(vertices);
	vbView.BufferLocation = vertexBuffer->GetGPUVirtualAddress();

	ibView.Format = DXGI_FORMAT_R32_UINT;
	ibView.SizeInBytes = sizeof(unsigned int) * ARRAYSIZE(indices);
	ibView.BufferLocation = indexBuffer->GetGPUVirtualAddress();
	*/
	// ^^ use meshes now. i should prob just delet this but thats a later problem
	D3D12_CPU_DESCRIPTOR_HANDLE cobblestoneAlbedo = dx12Helper.LoadTexture(L"cobblestone_albedo.png");
	D3D12_CPU_DESCRIPTOR_HANDLE cobblestoneMetal = dx12Helper.LoadTexture(L"cobblestone_metal.png");
	D3D12_CPU_DESCRIPTOR_HANDLE cobblestoneNormals = dx12Helper.LoadTexture(L"cobblestone_normals.png");
	D3D12_CPU_DESCRIPTOR_HANDLE cobblestoneRoughness = dx12Helper.LoadTexture(L"cobblestone_roughness.png");
	D3D12_CPU_DESCRIPTOR_HANDLE scratchedAlbedo = dx12Helper.LoadTexture(L"scratched_albedo.png");
	D3D12_CPU_DESCRIPTOR_HANDLE scratchedMetal = dx12Helper.LoadTexture(L"scratched_metal.png");
	D3D12_CPU_DESCRIPTOR_HANDLE scratchedNormals = dx12Helper.LoadTexture(L"scratched_normals.png");
	D3D12_CPU_DESCRIPTOR_HANDLE scratchedRoughness = dx12Helper.LoadTexture(L"scratched_roughness.png");

	std::shared_ptr<Material> cobbleMaterial = std::make_shared<Material>(pipelineState, XMFLOAT3(1, 0.5, 0.5), XMFLOAT2(1, 1), XMFLOAT2(0, 0));
	cobbleMaterial->AddTexture(cobblestoneAlbedo, 0);
	cobbleMaterial->AddTexture(cobblestoneNormals, 1);
	cobbleMaterial->AddTexture(cobblestoneMetal, 2);
	cobbleMaterial->AddTexture(cobblestoneRoughness, 3);
	cobbleMaterial->FinalizeMaterial();

	std::shared_ptr<Material> scratchedMaterial = std::make_shared<Material>(pipelineState, XMFLOAT3(0.5, 1, 0.5), XMFLOAT2(1, 1), XMFLOAT2(0, 0));
	scratchedMaterial->AddTexture(scratchedAlbedo, 0);
	scratchedMaterial->AddTexture(scratchedNormals, 1);
	scratchedMaterial->AddTexture(scratchedMetal, 2);
	scratchedMaterial->AddTexture(scratchedRoughness, 3);
	scratchedMaterial->FinalizeMaterial();

	meshList.push_back(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cube.obj").c_str()));
	meshList.push_back(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/cylinder.obj").c_str()));
	meshList.push_back(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/helix.obj").c_str()));
	meshList.push_back(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad.obj").c_str()));
	meshList.push_back(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/quad_double_sided.obj").c_str()));
	meshList.push_back(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/sphere.obj").c_str()));
	meshList.push_back(std::make_shared<Mesh>(FixPath(L"../../Assets/Models/torus.obj").c_str()));

	renderableList.push_back(Renderable(meshList[0], cobbleMaterial, XMFLOAT3(0, 0, 0)));
	renderableList.push_back(Renderable(meshList[1], cobbleMaterial, XMFLOAT3(0, 3, 0)));
	renderableList.push_back(Renderable(meshList[2], cobbleMaterial, XMFLOAT3(3, 0, 0)));
	renderableList.push_back(Renderable(meshList[0], cobbleMaterial, XMFLOAT3(-3, 3, 0)));
	renderableList.push_back(Renderable(meshList[4], scratchedMaterial, XMFLOAT3(3, 3, 0)));
	renderableList.push_back(Renderable(meshList[5], scratchedMaterial, XMFLOAT3(-3, 0, 0)));
	renderableList.push_back(Renderable(meshList[6], scratchedMaterial, XMFLOAT3(0, -3, 0)));

	// Meshes create their own BLAS's; we just need to create the TLAS for the scene here
	RaytracingHelper::GetInstance().CreateTopLevelAccelerationStructureForScene(renderableList);
}


// --------------------------------------------------------
// Handle resizing to match the new window size.
//  - DXCore needs to resize the back buffer
//  - Eventually, we'll want to update our 3D camera
// --------------------------------------------------------
void Game::OnResize()
{
	// Handle base-level DX resize stuff
	DXCore::OnResize();
	camera->SetAspect((float)(windowWidth / windowHeight));
	RaytracingHelper::GetInstance().ResizeOutputUAV(windowWidth, windowHeight);
}

// --------------------------------------------------------
// Update your game here - user input, move objects, AI, etc.
// --------------------------------------------------------
void Game::Update(float deltaTime, float totalTime)
{
	// Example input checking: Quit if the escape key is pressed
	if (Input::GetInstance().KeyDown(VK_ESCAPE))
		Quit();

	for (size_t i = 0; i < renderableList.size(); i++)
	{
		renderableList[i].GetTransform().Rotate(0.01f, 0.01f, 0.01f);
	}

	camera->Update(deltaTime);
}

// --------------------------------------------------------
// Clear the screen, redraw everything, present to the user
// --------------------------------------------------------
void Game::Draw(float deltaTime, float totalTime)
{
	// Grab the current back buffer for this frame
	Microsoft::WRL::ComPtr<ID3D12Resource> currentBackBuffer = backBuffers[currentSwapBuffer];

	// Clearing the render target
		/* not needed for ray tracing
	{
		// Transition the back buffer from present to render target
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);

		// Background color (Cornflower Blue in this case) for clearing
		float color[] = { 0.4f, 0.6f, 0.75f, 1.0f };

		// Clear the RTV
		commandList->ClearRenderTargetView(
			rtvHandles[currentSwapBuffer],
			color,
			0, 0); // No scissor rectangles
			

		// Clear the depth buffer, too, 
		commandList->ClearDepthStencilView(
			dsvHandle,
			D3D12_CLEAR_FLAG_DEPTH,
			1.0f, // Max depth = 1.0f
			0, // Not clearing stencil, but need a value
			0, 0); // No scissor rects
	}
	*/

	/*
	// Rendering here!
	{

		// Root sig (must happen before root descriptor table)
		commandList->SetGraphicsRootSignature(rootSignature.Get());
		
		// Descriptor heap
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap =
			dx12Helper.GetCBVSRVDescriptorHeap();
		commandList->SetDescriptorHeaps(1, descriptorHeap.GetAddressOf());

		// Set up other commands for rendering
		commandList->OMSetRenderTargets(1, &rtvHandles[currentSwapBuffer], true, &dsvHandle);
		commandList->RSSetViewports(1, &viewport);
		commandList->RSSetScissorRects(1, &scissorRect);
		commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

		// Draw
		for (size_t i = 0; i < renderableList.size(); i++)
		{
			std::shared_ptr<Material> mat = renderableList[i].GetMaterial();
			commandList->SetPipelineState(mat->GetPipelineState().Get());
			// Set the SRV descriptor handle for this material's textures
			// Note: This assumes that descriptor table 2 is for textures (as per our root sig)
			commandList->SetGraphicsRootDescriptorTable(2, mat->GetFinalGPUHandleForSRVs());
			
			// Pixel shader data and cbuffer setup
			{
				PixelShaderExternalData psData = {};
				psData.uvScale = mat->GetUVScale();
				psData.uvOffset = mat->GetUVOffset();
				psData.cameraPosition = camera->GetPosition();
				psData.lightCount = lightCount;
				memcpy(psData.lights, &lights[0], sizeof(Light) * 20);
				
				// Send this to a chunk of the constant buffer heap
				// and grab the GPU handle for it so we can set it for this draw
				D3D12_GPU_DESCRIPTOR_HANDLE cbHandlePS =
					dx12Helper.FillNextConstantBufferAndGetGPUDescriptorHandle(
						(void*)(&psData), sizeof(PixelShaderExternalData));
				
				// Set this constant buffer handle
				// Note: This assumes that descriptor table 1 is the
				// place to put this particular descriptor. This
				// is based on how we set up our root signature.
				commandList->SetGraphicsRootDescriptorTable(1, cbHandlePS);
			}
			
			VertexShaderExternalData vertexShaderData = {};
			vertexShaderData.world = renderableList[i].GetTransform().GetWorldMatrix();
			vertexShaderData.view = camera->GetView();
			vertexShaderData.projection = camera->GetProjection();
			vertexShaderData.worldInvTranspose = renderableList[i].GetTransform().GetWorldInverseTransposeMatrix();

			D3D12_GPU_DESCRIPTOR_HANDLE vsbDescriptorHandle = dx12Helper.FillNextConstantBufferAndGetGPUDescriptorHandle(&vertexShaderData, sizeof(VertexShaderExternalData));
			commandList->SetGraphicsRootDescriptorTable(0, vsbDescriptorHandle);

			D3D12_VERTEX_BUFFER_VIEW vbView = renderableList[i].GetMesh()->GetVBView();
			D3D12_INDEX_BUFFER_VIEW ibView = renderableList[i].GetMesh()->GetIBView();

			commandList->IASetVertexBuffers(0, 1, &vbView);
			commandList->IASetIndexBuffer(&ibView);

			commandList->DrawIndexedInstanced(renderableList[i].GetMesh()->GetIndexCount(), 1, 0, 0, 0);
		}
	}*/

	// Present
	{
		/*
		// Transition back to present
		D3D12_RESOURCE_BARRIER rb = {};
		rb.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
		rb.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
		rb.Transition.pResource = currentBackBuffer.Get();
		rb.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
		rb.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
		rb.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
		commandList->ResourceBarrier(1, &rb);
		// Must occur BEFORE present
		DX12Helper::GetInstance().CloseExecuteAndResetCommandList();
		*/

		
		// Update raytracing accel structure
		RaytracingHelper::GetInstance().
			CreateTopLevelAccelerationStructureForScene(renderableList);

		// Perform raytrace, including execution of command list
		RaytracingHelper::GetInstance().Raytrace(
			camera,
			backBuffers[currentSwapBuffer]);

		// Always wait before reseting command allocator, as it should not
		// be reset while the GPU is processing a command list
		// See: https://docs.microsoft.com/en-us/windows/desktop/api/d3d12/nf-d3d12-id3d12commandallocator-reset
		DX12Helper::GetInstance().WaitForGPU();
		commandAllocator->Reset();
		commandList->Reset(commandAllocator.Get(), 0);
		

		// Present the current back buffer
		bool vsyncNecessary = vsync || !deviceSupportsTearing || isFullscreen;
		swapChain->Present(
			vsyncNecessary ? 1 : 0,
			vsyncNecessary ? 0 : DXGI_PRESENT_ALLOW_TEARING);
		// Figure out which buffer is next
		currentSwapBuffer++;
		if (currentSwapBuffer >= numBackBuffers)
			currentSwapBuffer = 0;
	}
}