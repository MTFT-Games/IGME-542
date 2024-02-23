#pragma once
#include <DirectXMath.h>
#include "DXCore.h"
#include <wrl/client.h>
class Material
{
	// TODO: Add more usefull things from the dx11 version
public:
	Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState, DirectX::XMFLOAT3 colorTint, DirectX::XMFLOAT2 uvScale, DirectX::XMFLOAT2 uvOffset);

	// albedo, normal, metal, rough, in that order
	void AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot);
	void FinalizeMaterial();

	Microsoft::WRL::ComPtr<ID3D12PipelineState> GetPipelineState();
	D3D12_GPU_DESCRIPTOR_HANDLE GetFinalGPUHandleForSRVs();
	DirectX::XMFLOAT2 GetUVScale();
	DirectX::XMFLOAT2 GetUVOffset();

private:
	DirectX::XMFLOAT3 colorTint;
	DirectX::XMFLOAT2 uvScale;
	DirectX::XMFLOAT2 uvOffset;
	bool finalized;

	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState;
	// TODO: This could be more flexible up to 128 if needed
	D3D12_CPU_DESCRIPTOR_HANDLE textureSRVsBySlot [4];
	D3D12_GPU_DESCRIPTOR_HANDLE finalGPUHandleForSRVs;
};

