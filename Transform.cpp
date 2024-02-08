#include "Transform.h"

using namespace DirectX;

Transform::Transform():
    position(XMFLOAT3()),
    scale(XMFLOAT3(1, 1, 1)),
    pitchYawRoll(XMFLOAT3()),
    dirty(false)
{
    XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixIdentity());
}

Transform::Transform(DirectX::XMFLOAT3 _position, DirectX::XMFLOAT3 _scale, DirectX::XMFLOAT3 _pitchYawRoll) :
    position(_position),
    scale(_scale),
    pitchYawRoll(_pitchYawRoll),
    dirty(true)
{
    // Matricies are dirty and will be calculated upon retrieval in case more changes are made before then
    // This is to make the compiler warnings shut up
    XMStoreFloat4x4(&worldMatrix, XMMatrixIdentity());
    XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixIdentity());
}

void Transform::SetPosition(float x, float y, float z)
{
    position = XMFLOAT3(x, y, z);
    dirty = true;
}

void Transform::SetPosition(DirectX::XMFLOAT3 _position)
{
    position = _position;
    dirty = true;
}

void Transform::SetRotation(float pitch, float yaw, float roll)
{
    pitchYawRoll = XMFLOAT3(pitch, yaw, roll);
    dirty = true;
}

void Transform::setRotation(DirectX::XMFLOAT3 rotation)
{
    pitchYawRoll = rotation;
    dirty = true;
}

void Transform::SetScale(float x, float y, float z)
{
    scale = XMFLOAT3(x, y, z);
    dirty = true;
}

void Transform::SetScale(float _scale)
{
    scale = XMFLOAT3(_scale, _scale, _scale);
    dirty = true;
}

void Transform::SetScale(DirectX::XMFLOAT3 _scale)
{
    scale = _scale;
    dirty = true;
}

DirectX::XMFLOAT3 Transform::GetPosition()
{
    return position;
}

DirectX::XMFLOAT3 Transform::GetPitchYawRoll()
{
    return pitchYawRoll;
}

DirectX::XMFLOAT3 Transform::GetScale()
{
    return scale;
}

DirectX::XMFLOAT4X4 Transform::GetWorldMatrix()
{
    if (dirty) GenerateMatricies();

    return worldMatrix;
}

DirectX::XMFLOAT4X4 Transform::GetWorldInverseTransposeMatrix()
{
    if (dirty) GenerateMatricies();

    return worldInverseTransposeMatrix;
}

DirectX::XMFLOAT3 Transform::GetRight()
{
    XMVECTOR right = XMVectorSet(1, 0, 0, 0);
    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z); // TODO: What if i make a similar get quaternion to the world matrix so i dont need to keep re converting from roll pitch yaw. or honelstly same with the direction vectors... hmm more memory for probably little performance saving. sounds like a later problem
    right = XMVector3Rotate(right, quaternion);
    XMFLOAT3 result;
    XMStoreFloat3(&result, right);
    return result;
}

DirectX::XMFLOAT3 Transform::GetUp()
{
    XMVECTOR up = XMVectorSet(0, 1, 0, 0);
    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z); 
    up = XMVector3Rotate(up, quaternion);
    XMFLOAT3 result;
    XMStoreFloat3(&result, up);
    return result;
}

DirectX::XMFLOAT3 Transform::GetForward()
{
    XMVECTOR forward = XMVectorSet(0, 0, 1, 0);
    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
    forward = XMVector3Rotate(forward, quaternion);
    XMFLOAT3 result;
    XMStoreFloat3(&result, forward);
    return result;
}

void Transform::MoveAbsolute(float x, float y, float z)
{
    XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), XMVectorSet(x, y, z, 0)));
    dirty = true;
}

void Transform::MoveAbsolute(DirectX::XMFLOAT3 offset)
{
    XMStoreFloat3(&position, XMVectorAdd(XMLoadFloat3(&position), XMLoadFloat3(&offset)));
    dirty = true;
}

void Transform::Rotate(float pitch, float yaw, float roll)
{
    XMStoreFloat3(&pitchYawRoll, XMVectorAdd(XMLoadFloat3(&pitchYawRoll), XMVectorSet(pitch, yaw, roll, 0)));
    dirty = true;
}

void Transform::Rotate(DirectX::XMFLOAT3 rotation)
{
    XMStoreFloat3(&pitchYawRoll, XMVectorAdd(XMLoadFloat3(&pitchYawRoll), XMLoadFloat3(&rotation)));
    dirty = true;
}

void Transform::Scale(float x, float y, float z)
{
    XMStoreFloat3(&scale, XMVectorMultiply(XMLoadFloat3(&scale), XMVectorSet(x, y, z, 1)));
    dirty = true;
}

void Transform::Scale(DirectX::XMFLOAT3 _scale)
{
    XMStoreFloat3(&scale, XMVectorMultiply(XMLoadFloat3(&scale), XMLoadFloat3(&_scale)));
    dirty = true;
}

void Transform::Scale(float _scale)
{
    XMStoreFloat3(&scale, XMVectorMultiply(XMLoadFloat3(&scale), XMVectorSet(_scale, _scale, _scale, 1)));
    dirty = true;
}

void Transform::MoveRelative(float x, float y, float z)
{
    XMVECTOR move = XMVectorSet(x, y, z, 0);
    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
    move = XMVector3Rotate(move, quaternion);
    XMStoreFloat3(&position, XMVectorAdd(move, XMLoadFloat3(&position)));
}

void Transform::MoveRelative(DirectX::XMFLOAT3 offset)
{
    XMVECTOR move = XMLoadFloat3(&offset);
    XMVECTOR quaternion = XMQuaternionRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
    move = XMVector3Rotate(move, quaternion);
    XMStoreFloat3(&position, XMVectorAdd(move, XMLoadFloat3(&position)));
}

void Transform::GenerateMatricies()
{
    XMMATRIX translation = XMMatrixTranslation(position.x, position.y, position.z);
    XMMATRIX rotation = XMMatrixRotationRollPitchYaw(pitchYawRoll.x, pitchYawRoll.y, pitchYawRoll.z);
    XMMATRIX scaling = XMMatrixScaling(scale.x, scale.y, scale.z);

    // multiplied reverse for GPU
    XMMATRIX world = XMMatrixMultiply(XMMatrixMultiply(scaling, rotation), translation);

    XMStoreFloat4x4(&worldMatrix, world);
    // For a future thing
    XMStoreFloat4x4(&worldInverseTransposeMatrix, XMMatrixInverse(0, XMMatrixTranspose(world)));

    dirty = false;
}
