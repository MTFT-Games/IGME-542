#pragma once
#include "Transform.h"
#include "Mesh.h"
#include <memory>
#include "Camera.h"
#include "Material.h"

class Renderable
{
public:
	Renderable(std::shared_ptr<Mesh> _mesh, std::shared_ptr<Material> material, DirectX::XMFLOAT3 position = DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3 rotation = DirectX::XMFLOAT3(0, 0, 0), DirectX::XMFLOAT3 scale = DirectX::XMFLOAT3(1, 1, 1));

	std::shared_ptr<Mesh> GetMesh();
	Transform& GetTransform();

	void SetMaterial(std::shared_ptr<Material> material);
	std::shared_ptr<Material> GetMaterial();

	//void Draw(std::shared_ptr<Camera> camera);

private:
	Transform transform;
	std::shared_ptr<Mesh> mesh;
	std::shared_ptr<Material> material;
};

