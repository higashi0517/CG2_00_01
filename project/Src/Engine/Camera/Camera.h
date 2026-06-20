#pragma once
#include <Matrix4x4.h>

class Camera
{
private:
	Transform transform;
	Matrix4x4 worldMatrix;
	Matrix4x4 viewMatrix;
	Matrix4x4 projectionMatrix;
	Matrix4x4 viewProjectionMatrix;
	float fovY;
	float aspectRatio;
	float nearClip;
	float farClip;
public:
	void Update();
	Camera();

	// setter
	void SetTranslate(const Vector3& translate_) { this->transform.translate = translate_; }
	void SetRotate(const Vector3& rotate_) { this->transform.rotate = rotate_; }
	void SetFovY(float fovY_) { this->fovY = fovY_; }
	void SetAspectRatio(float aspectRatio_) { this->aspectRatio = aspectRatio_; }
	void SetNearClip(float nearClip_) { this->nearClip = nearClip_; }
	void SetFarClip(float farClip_) { this->farClip = farClip_; }

	// getter
	const Matrix4x4& GetViewMatrix() const { return viewMatrix; }
	const Matrix4x4& GetProjectionMatrix() const { return projectionMatrix; }
	const Matrix4x4& GetViewProjectionMatrix() const { return viewProjectionMatrix; }
	const Matrix4x4& GetWorldMatrix() const { return worldMatrix; }
	const Vector3 GetTranslate() const { return transform.translate; }
	const Vector3 GetRotate() const { return transform.rotate; }
};

