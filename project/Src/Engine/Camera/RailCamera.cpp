#include "RailCamera.h"

#include <algorithm>
#include <cmath>

#ifdef _DEBUG
#include "imgui.h"
#endif

void RailCamera::Initialize(
	Camera* camera,
	const std::vector<Vector3>& controlPoints)
{
	camera_ = camera;
	controlPoints_ = controlPoints;
	progress_ = 0.0f;
}

void RailCamera::Update()
{
	if (!isActive_ || camera_ == nullptr || controlPoints_.size() < 4) {
		return;
	}

	railPosition_ = CalculatePosition(progress_);

	float aheadProgress = progress_ + 0.002f;
	if (isLoop_) {
		aheadProgress -= std::floor(aheadProgress);
	}
	else {
		aheadProgress = std::min(aheadProgress, 1.0f);
	}

	const Vector3 aheadPosition = CalculatePosition(aheadProgress);

	const Vector3 direction = {
		aheadPosition.x - railPosition_.x,
		aheadPosition.y - railPosition_.y,
		aheadPosition.z - railPosition_.z
	};
	const float directionLength = std::sqrt(
		direction.x * direction.x +
		direction.y * direction.y +
		direction.z * direction.z
	);

	if (directionLength > 0.0001f) {
		forward_ = {
			direction.x / directionLength,
			direction.y / directionLength,
			direction.z / directionLength
		};
	}

	const float rightLength = std::sqrt(
		forward_.x * forward_.x +
		forward_.z * forward_.z
	);
	if (rightLength > 0.0001f) {
		right_ = {
			forward_.z / rightLength,
			0.0f,
			-forward_.x / rightLength
		};
	}

	const Vector3 cameraPosition = {
		railPosition_.x - forward_.x * cameraDistance_,
		railPosition_.y - forward_.y * cameraDistance_ + cameraHeight_,
		railPosition_.z - forward_.z * cameraDistance_
	};
	const Vector3 cameraTarget = {
		railPosition_.x + forward_.x * 5.0f,
		railPosition_.y + forward_.y * 5.0f + 0.5f,
		railPosition_.z + forward_.z * 5.0f
	};
	const Vector3 cameraDirection = {
		cameraTarget.x - cameraPosition.x,
		cameraTarget.y - cameraPosition.y,
		cameraTarget.z - cameraPosition.z
	};

	const float horizontalLength = std::sqrt(
		cameraDirection.x * cameraDirection.x +
		cameraDirection.z * cameraDirection.z
	);

	const float pitch = -std::atan2(cameraDirection.y, horizontalLength);
	const float yaw = std::atan2(cameraDirection.x, cameraDirection.z);
	const float railHorizontalLength = std::sqrt(
		forward_.x * forward_.x +
		forward_.z * forward_.z
	);
	railRotation_ = {
		-std::atan2(forward_.y, railHorizontalLength),
		std::atan2(forward_.x, forward_.z),
		0.0f
	};

	camera_->SetTranslate(cameraPosition);
	camera_->SetRotate({ pitch, yaw, 0.0f });
	camera_->Update();

	progress_ += speed_;
	if (isLoop_) {
		progress_ -= std::floor(progress_);
	}
	else {
		progress_ = std::min(progress_, 1.0f);
	}
}

void RailCamera::DrawDebug(
	const Matrix4x4& viewProjectionMatrix,
	float screenWidth,
	float screenHeight) const
{
#ifdef _DEBUG
	if (controlPoints_.size() < 4) {
		return;
	}

	auto worldToScreen = [
		&viewProjectionMatrix,
		screenWidth,
		screenHeight
	](const Vector3& position, ImVec2& screenPosition)
		{
			const float clipX =
				position.x * viewProjectionMatrix.m[0][0] +
				position.y * viewProjectionMatrix.m[1][0] +
				position.z * viewProjectionMatrix.m[2][0] +
				viewProjectionMatrix.m[3][0];
			const float clipY =
				position.x * viewProjectionMatrix.m[0][1] +
				position.y * viewProjectionMatrix.m[1][1] +
				position.z * viewProjectionMatrix.m[2][1] +
				viewProjectionMatrix.m[3][1];
			const float clipZ =
				position.x * viewProjectionMatrix.m[0][2] +
				position.y * viewProjectionMatrix.m[1][2] +
				position.z * viewProjectionMatrix.m[2][2] +
				viewProjectionMatrix.m[3][2];
			const float clipW =
				position.x * viewProjectionMatrix.m[0][3] +
				position.y * viewProjectionMatrix.m[1][3] +
				position.z * viewProjectionMatrix.m[2][3] +
				viewProjectionMatrix.m[3][3];

			if (clipW <= 0.0f) {
				return false;
			}

			const float ndcX = clipX / clipW;
			const float ndcY = clipY / clipW;
			const float ndcZ = clipZ / clipW;
			if (ndcZ < 0.0f || ndcZ > 1.0f) {
				return false;
			}

			screenPosition.x = (ndcX + 1.0f) * 0.5f * screenWidth;
			screenPosition.y = (1.0f - ndcY) * 0.5f * screenHeight;
			return true;
		};

	ImDrawList* drawList = ImGui::GetForegroundDrawList();
	const ImU32 curveColor = IM_COL32(0, 220, 255, 255);
	const ImU32 pointColor = IM_COL32(255, 140, 0, 255);
	const ImU32 currentColor = IM_COL32(255, 255, 0, 255);

	constexpr int kSampleCount = 160;
	ImVec2 previousScreen{};
	bool previousVisible = false;

	for (int i = 0; i <= kSampleCount; ++i) {
		const float sampleProgress =
			static_cast<float>(i) / static_cast<float>(kSampleCount);
		const Vector3 samplePosition = CalculatePosition(sampleProgress);
		ImVec2 currentScreen{};
		const bool currentVisible =
			worldToScreen(samplePosition, currentScreen);

		if (previousVisible && currentVisible) {
			drawList->AddLine(
				previousScreen,
				currentScreen,
				curveColor,
				3.0f);
		}

		previousScreen = currentScreen;
		previousVisible = currentVisible;
	}

	for (const Vector3& controlPoint : controlPoints_) {
		ImVec2 screenPosition{};
		if (worldToScreen(controlPoint, screenPosition)) {
			drawList->AddCircleFilled(screenPosition, 5.0f, pointColor);
		}
	}

	ImVec2 currentScreen{};
	if (worldToScreen(railPosition_, currentScreen)) {
		drawList->AddCircleFilled(currentScreen, 7.0f, currentColor);
	}
#else
	(void)viewProjectionMatrix;
	(void)screenWidth;
	(void)screenHeight;
#endif
}

Vector3 RailCamera::CalculatePosition(float progress) const
{
	if (controlPoints_.size() < 4) {
		return { 0.0f, 0.0f, 0.0f };
	}

	if (isLoop_) {
		const size_t pointCount = controlPoints_.size();
		const float railPosition = progress * static_cast<float>(pointCount);
		const size_t index1 = static_cast<size_t>(railPosition) % pointCount;
		const size_t index0 = (index1 + pointCount - 1) % pointCount;
		const size_t index2 = (index1 + 1) % pointCount;
		const size_t index3 = (index1 + 2) % pointCount;
		const float localT = railPosition - std::floor(railPosition);

		return CatmullRom(
			controlPoints_[index0],
			controlPoints_[index1],
			controlPoints_[index2],
			controlPoints_[index3],
			localT
		);
	}

	const size_t segmentCount = controlPoints_.size() - 3;
	const float railPosition =
		std::clamp(progress, 0.0f, 1.0f) *
		static_cast<float>(segmentCount);
	const size_t segmentIndex = std::min(
		static_cast<size_t>(railPosition),
		segmentCount - 1
	);
	const float localT = std::min(
		railPosition - static_cast<float>(segmentIndex),
		1.0f
	);

	return CatmullRom(
		controlPoints_[segmentIndex],
		controlPoints_[segmentIndex + 1],
		controlPoints_[segmentIndex + 2],
		controlPoints_[segmentIndex + 3],
		localT
	);
}

Vector3 RailCamera::CatmullRom(
	const Vector3& p0,
	const Vector3& p1,
	const Vector3& p2,
	const Vector3& p3,
	float t)
{
	const float t2 = t * t;
	const float t3 = t2 * t;

	return {
		0.5f * ((2.0f * p1.x) +
			(-p0.x + p2.x) * t +
			(2.0f * p0.x - 5.0f * p1.x + 4.0f * p2.x - p3.x) * t2 +
			(-p0.x + 3.0f * p1.x - 3.0f * p2.x + p3.x) * t3),
		0.5f * ((2.0f * p1.y) +
			(-p0.y + p2.y) * t +
			(2.0f * p0.y - 5.0f * p1.y + 4.0f * p2.y - p3.y) * t2 +
			(-p0.y + 3.0f * p1.y - 3.0f * p2.y + p3.y) * t3),
		0.5f * ((2.0f * p1.z) +
			(-p0.z + p2.z) * t +
			(2.0f * p0.z - 5.0f * p1.z + 4.0f * p2.z - p3.z) * t2 +
			(-p0.z + 3.0f * p1.z - 3.0f * p2.z + p3.z) * t3)
	};
}
