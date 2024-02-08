#include "Renderable.h"

Renderable::Renderable(std::shared_ptr<Mesh> _mesh, /*std::shared_ptr<Material> _material,*/ DirectX::XMFLOAT3 position, DirectX::XMFLOAT3 rotation, DirectX::XMFLOAT3 scale) :
    mesh(_mesh)//, material(_material)
{
	transform.SetPosition(position);
	transform.setRotation(rotation);
	transform.SetScale(scale);
}

std::shared_ptr<Mesh> Renderable::GetMesh()
{
    return mesh;
}

Transform& Renderable::GetTransform()
{
    return transform;
}

/*void Renderable::SetMaterial(std::shared_ptr<Material> _material)
{
	material = _material;
}

std::shared_ptr<Material> Renderable::GetMaterial()
{
	return material;
}

void Renderable::Draw(std::shared_ptr<Camera> camera)
{
	// Set material shaders TODO: A better system could optimize this with all renderables with the same shaders grouped to prevent excessive switching	
	material->PrepareMaterial();

	// Constant buffers Data
	std::shared_ptr<SimpleVertexShader> vs = material->GetVertexShader();
	std::shared_ptr<SimplePixelShader> ps = material->GetPixelShader();

	ps->SetFloat3("cameraPosition", camera->GetPosition());
	vs->SetMatrix4x4("worldMatrix", transform.GetWorldMatrix());
	vs->SetMatrix4x4("viewMatrix", camera->GetView());
	vs->SetMatrix4x4("projectionMatrix", camera->GetProjection());
	vs->SetMatrix4x4("worldInvTranspose", transform.GetWorldInverseTransposeMatrix());

	// Map and copy data
	vs->CopyAllBufferData();
	ps->CopyAllBufferData();

	mesh->Draw();
}*/
