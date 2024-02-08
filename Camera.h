#pragma once
#include "Input.h"
#include "Transform.h"

class Camera
{
	// TODO: Add a crap ton more comments
public:
	// Constructor
	Camera(
		float aspectRatio,
		DirectX::XMFLOAT3 position = DirectX::XMFLOAT3(0, 0, 0), 
		DirectX::XMFLOAT3 rotation = DirectX::XMFLOAT3(0, 0, 0),
		float fov = 3.14/2,
		bool ortho = false,
		float nearClip = 0.01,
		float farClip = 1000,
		float sensetivity = 0.001, // TODO: Test and find reasonable default
		float speed = 1); // TODO: Add more overloads for taking seperate x y z and others

	// Getters

	// Needed for shaders later
	DirectX::XMFLOAT4X4 GetView();
	DirectX::XMFLOAT4X4 GetProjection();
	DirectX::XMFLOAT3 GetPosition();

	// Setters

	void SetAspect(float aspectRatio);
	// TODO: Make setters for other settings. Not needed for assignment but should do when i have free time
	/*
	void SetFovRad(float fovRadian);
	void SetFovDeg(float fovDegrees);
	void SetClip(float near, float far);
	void SetClipFar(float farClip);
	void SetSensitivity(float sensetivity);
	void SetMoveSpeed(float speed);
	void SetOrtho(bool orthographic);
	*/

	// TODO: Add methods to move the camera from outside the camera class (
	// say for the purposes of a cutscene or event in game). Cant just give the transform
	// reference because we wont know if its changed and we need to update matricies

	// Settings that dont need to do anything special when changed

	float mouseSenetivity;
	float moveSpeed;


	// Process player input
	void Update(float dt);

	// Show Imgui stats for this camera
	void ImGuiUpdate();

private:
	// Recalculates the projection matrix for any new settings
	void UpdateProjectionMatrix();

	// Recalculates the view matrix for any changes in the transform
	void UpdateViewMatrix();

	// Fields

	Transform transform;
	DirectX::XMFLOAT4X4 viewMatrix;
	DirectX::XMFLOAT4X4 projectionMatrix;

	// Settings (Private with setters and getters so matricies get updated on change)

	float fov;
	float nearClip;
	float farClip;
	bool ortho;
	float aspectRatio;
};

