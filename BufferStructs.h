#pragma once
#include <DirectXMath.h>

struct VertexShaderExternalData
{
    DirectX::XMFLOAT4X4 world;
    DirectX::XMFLOAT4X4 view;
    DirectX::XMFLOAT4X4 projection;
    DirectX::XMFLOAT4X4 worldInvTranspose;
};
struct PixelShaderExternalData
{
    DirectX::XMFLOAT2 uvScale;
    DirectX::XMFLOAT2 uvOffset;
    DirectX::XMFLOAT3 cameraPosition;
    int lightCount;
    Light lights[20];
};