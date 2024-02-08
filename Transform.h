#pragma once
#include <DirectXMath.h>

// Holds a global position, rotation, and scale with a combined world matrix.
class Transform
{

public:
	Transform();
	Transform(
		DirectX::XMFLOAT3 _position, 
		DirectX::XMFLOAT3 _scale = DirectX::XMFLOAT3(1, 1, 1), 
		DirectX::XMFLOAT3 _pitchYawRoll = DirectX::XMFLOAT3());

	// Setters

	void SetPosition(float x, float y, float z);
	void SetPosition(DirectX::XMFLOAT3 _position);
	void SetRotation(float pitch, float yaw, float roll);
	void setRotation(DirectX::XMFLOAT3 rotation);
	void SetScale(float x, float y, float z);
	void SetScale(float _scale);
	void SetScale(DirectX::XMFLOAT3 _scale);

	// Getters
	
	DirectX::XMFLOAT3 GetPosition();
	DirectX::XMFLOAT3 GetPitchYawRoll();
	DirectX::XMFLOAT3 GetScale();
	DirectX::XMFLOAT4X4 GetWorldMatrix();
	DirectX::XMFLOAT4X4 GetWorldInverseTransposeMatrix();
	DirectX::XMFLOAT3 GetRight();
	DirectX::XMFLOAT3 GetUp();
	DirectX::XMFLOAT3 GetForward();

	// Transformers

	void MoveAbsolute(float x, float y, float z);
	void MoveAbsolute(DirectX::XMFLOAT3 offset);
	void Rotate(float pitch, float yaw, float roll);
	void Rotate(DirectX::XMFLOAT3 rotation);
	void Scale(float x, float y, float z);
	void Scale(DirectX::XMFLOAT3 _scale);
	void Scale(float _scale);
	void MoveRelative(float x, float y, float z);
	void MoveRelative(DirectX::XMFLOAT3 offset);

private:

	DirectX::XMFLOAT3 position;
	DirectX::XMFLOAT3 scale;
	DirectX::XMFLOAT3 pitchYawRoll;

	DirectX::XMFLOAT4X4 worldMatrix;
	DirectX::XMFLOAT4X4 worldInverseTransposeMatrix;
	bool dirty;

	void GenerateMatricies();
};

