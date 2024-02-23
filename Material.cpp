#include "Material.h"
#include "DX12Helper.h"

Material::Material(Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState, DirectX::XMFLOAT3 colorTint, DirectX::XMFLOAT2 uvScale, DirectX::XMFLOAT2 uvOffset)
	:colorTint(colorTint), pipelineState(pipelineState), uvScale(uvScale), uvOffset(uvOffset), finalized(false), finalGPUHandleForSRVs(D3D12_GPU_DESCRIPTOR_HANDLE()), textureSRVsBySlot()
{
}

void Material::AddTexture(D3D12_CPU_DESCRIPTOR_HANDLE srv, int slot)
{
	if (slot > 3 || finalized)
	{
		return;
	}
	textureSRVsBySlot[slot] = srv;
}

void Material::FinalizeMaterial()
{
	if (finalized) return;

	finalGPUHandleForSRVs =  DX12Helper::GetInstance().CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[0], 1);
	for (size_t i = 1; i < 4; i++)
	{
		DX12Helper::GetInstance().CopySRVsToDescriptorHeapAndGetGPUDescriptorHandle(textureSRVsBySlot[i], 1);
	}
}

Microsoft::WRL::ComPtr<ID3D12PipelineState> Material::GetPipelineState()
{
	return pipelineState;
}

D3D12_GPU_DESCRIPTOR_HANDLE Material::GetFinalGPUHandleForSRVs()
{
	return finalGPUHandleForSRVs;
}

DirectX::XMFLOAT2 Material::GetUVScale()
{
	return uvScale;
}

DirectX::XMFLOAT2 Material::GetUVOffset()
{
	return uvOffset;
}
