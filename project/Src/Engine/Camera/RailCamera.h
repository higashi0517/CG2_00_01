#pragma once

#include "Camera.h"
#include <vector>

// 複数の制御点を滑らかに通過するレールカメラ
class RailCamera
{
public:
	void Initialize(Camera* camera, const std::vector<Vector3>& controlPoints);
	void Update();
	void DrawDebug(
		const Matrix4x4& viewProjectionMatrix,
		float screenWidth,
		float screenHeight) const;

	void SetSpeed(float speed) { speed_ = speed; }
	void SetLoop(bool isLoop) { isLoop_ = isLoop; }
	void SetActive(bool isActive) { isActive_ = isActive; }
	void ToggleActive() { isActive_ = !isActive_; }
	bool IsActive() const { return isActive_; }
	void Reset() { progress_ = 0.0f; }
	float GetProgress() const { return progress_; }
	Vector3 GetPositionAtProgress(float progress) const { return CalculatePosition(progress); }
	const Vector3& GetRailPosition() const { return railPosition_; }
	const Vector3& GetForward() const { return forward_; }
	const Vector3& GetRight() const { return right_; }
	const Vector3& GetRailRotation() const { return railRotation_; }

private:
	Vector3 CalculatePosition(float progress) const;
	static Vector3 CatmullRom(
		const Vector3& p0,
		const Vector3& p1,
		const Vector3& p2,
		const Vector3& p3,
		float t
	);

	Camera* camera_ = nullptr;
	std::vector<Vector3> controlPoints_;
	Vector3 railPosition_ = { 0.0f, 0.0f, 0.0f };
	Vector3 forward_ = { 0.0f, 0.0f, 1.0f };
	Vector3 right_ = { 1.0f, 0.0f, 0.0f };
	Vector3 railRotation_ = { 0.0f, 0.0f, 0.0f };
	float progress_ = 0.0f;
	float speed_ = 0.0005f;
	float cameraDistance_ = 8.0f;
	float cameraHeight_ = 3.0f;
	bool isLoop_ = true;
	bool isActive_ = false;
};
