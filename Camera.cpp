#include "Camera.h"
#include "Input.h"
#include "ImGui/imgui.h"

using namespace DirectX;

Camera::Camera(
    float _aspectRatio,
    DirectX::XMFLOAT3 position,
    DirectX::XMFLOAT3 rotation,
    float _fov,
    bool _ortho,
    float _nearClip,
    float _farClip,
    float sensetivity,
    float speed) :
    aspectRatio(_aspectRatio),
    fov(_fov),
    ortho(_ortho),
    nearClip(_nearClip),
    farClip(_farClip),
    mouseSenetivity(sensetivity),
    moveSpeed(speed)
{
    transform.SetPosition(position);
    transform.setRotation(rotation);

    Camera::UpdateViewMatrix();
    Camera::UpdateProjectionMatrix();
}

DirectX::XMFLOAT4X4 Camera::GetView()
{
    return viewMatrix;
}

DirectX::XMFLOAT4X4 Camera::GetProjection()
{
    return projectionMatrix;
}

DirectX::XMFLOAT3 Camera::GetPosition()
{
    return transform.GetPosition();
}

void Camera::SetAspect(float _aspectRatio)
{
    aspectRatio = _aspectRatio;
    Camera::UpdateProjectionMatrix(); // TODO: Only recalculate when needed (changes were made and its needed)
}

void Camera::Update(float dt)
{
    Input& input = Input::GetInstance();
    bool moved = false;

    // TODO: Make diagonal not faster
    // TODO: Combine movements and edit the transform once
    // TODO: Add speed modifiers
    if (input.KeyDown('W')) 
    {
        transform.MoveRelative(0, 0, moveSpeed * dt);
        moved = true;
    }
    if (input.KeyDown('A'))
    {
        transform.MoveRelative(-moveSpeed * dt,0,0);
        moved = true;
    }
    if (input.KeyDown('S'))
    {
        transform.MoveRelative(0, 0, -moveSpeed * dt);
        moved = true;
    }
    if (input.KeyDown('D'))
    {
        transform.MoveRelative(moveSpeed * dt,0,0);
        moved = true;
    }
    if (input.KeyDown('W'))
    {
        transform.MoveRelative(0, 0, moveSpeed * dt);
        moved = true;
    }
    if (input.KeyDown(' '))
    {
        transform.MoveAbsolute(0, moveSpeed * dt,0);
        moved = true;
    }
    if (input.KeyDown('C'))
    {
        transform.MoveAbsolute(0, -moveSpeed * dt,0);
        moved = true;
    }

    if (input.MouseLeftDown())
    {
        float yawChange = input.GetMouseXDelta() * mouseSenetivity;
        float pitchChange = input.GetMouseYDelta() * mouseSenetivity;

        transform.Rotate(pitchChange, yawChange, 0);

        // TODO: I feel like theres a better way
        XMFLOAT3 newRot = transform.GetPitchYawRoll();
        if (newRot.x > 3.14/2)
        {
            newRot.x = 3.14f / 2;
            transform.setRotation(newRot);
        } else if (newRot.x < -3.14 / 2)
        {
            newRot.x = -3.14f / 2;
            transform.setRotation(newRot);
        }
        moved = true;
    }

    if (moved)
    {
        Camera::UpdateViewMatrix();
    }
}

void Camera::ImGuiUpdate()
{
    XMFLOAT3 camPos = transform.GetPosition();
    ImGui::Text("Position: %f, %f, %f", camPos.x, camPos.y, camPos.z);
    XMFLOAT3 camDir = transform.GetForward();
    ImGui::Text("Rotation: %f, %f, %f", camDir.x, camDir.y, camDir.z);
    ImGui::Text("Fov: %f", fov);
    ImGui::Text("Clip planes: %f, %f", nearClip, farClip);
    ImGui::Text("Orthographic: %d", ortho);
    ImGui::Text("Aspect ratio: %f", aspectRatio);
    ImGui::Text("Sensetivity: %f", mouseSenetivity);

}

void Camera::UpdateProjectionMatrix()
{
    if (ortho)
    {
        XMStoreFloat4x4(&projectionMatrix, XMMatrixOrthographicLH(fov, fov / aspectRatio, nearClip, farClip));
        return;
    }
    XMStoreFloat4x4(&projectionMatrix, XMMatrixPerspectiveFovLH(fov, aspectRatio, nearClip, farClip));
}

void Camera::UpdateViewMatrix()
{
    XMFLOAT3 transformPos = transform.GetPosition();
    XMFLOAT3 transformFor = transform.GetForward();

    XMVECTOR pos = XMLoadFloat3(&transformPos);
    XMVECTOR dir = XMLoadFloat3(&transformFor);

    // TODO: Make a setting to allow roll and if set use the transforms up
    XMStoreFloat4x4(&viewMatrix, XMMatrixLookToLH(pos, dir, XMVectorSet(0, 1, 0, 0)));
}
