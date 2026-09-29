#pragma once

#include "Matrix4x4.h"
#include <array>

#ifdef _DEBUG
#include "imgui.h"
#endif
#include <cstdint>

namespace CollisionLayer {
	constexpr uint32_t Player = 1u << 0;
	constexpr uint32_t Enemy = 1u << 1;
	constexpr uint32_t PlayerBullet = 1u << 2;
	constexpr uint32_t EnemyBullet = 1u << 3;
}

class Collider
{
public:
	struct Bounds
	{
		Vector3 min;
		Vector3 max;
	};

	void SetAttribute(uint32_t attribute) {
		attribute_ = attribute;
	}

	void SetMask(uint32_t mask) {
		mask_ = mask;
	}

	Collider()
	{
		UpdateBounds();
	}

	void SetCenter(const Vector3& center)
	{
		center_ = center;
		UpdateBounds();
	}

	void SetHalfSize(const Vector3& halfSize)
	{
		halfSize_ = halfSize;
		UpdateBounds();
	}

	const Vector3& GetCenter() const
	{
		return center_;
	}

	const Vector3& GetHalfSize() const
	{
		return halfSize_;
	}

	const Bounds& GetBounds() const
	{
		return bounds_;
	}

	bool IsCollision(const Collider& other) const {

		// お互いの属性が対象外なら判定しない
		if ((mask_ & other.attribute_) == 0 ||
			(other.mask_ & attribute_) == 0) {
			return false;
		}

		// ここから今までのAABB判定
		return
			bounds_.min.x <= other.bounds_.max.x &&
			bounds_.max.x >= other.bounds_.min.x &&
			bounds_.min.y <= other.bounds_.max.y &&
			bounds_.max.y >= other.bounds_.min.y &&
			bounds_.min.z <= other.bounds_.max.z &&
			bounds_.max.z >= other.bounds_.min.z;
	}

	void Draw(
		const Matrix4x4& viewProjectionMatrix,
		float screenWidth,
		float screenHeight,
		const Vector4& color) const
	{
#ifdef _DEBUG
		const std::array<Vector3, 8> corners = {
			Vector3{ bounds_.min.x, bounds_.min.y, bounds_.min.z },
			Vector3{ bounds_.max.x, bounds_.min.y, bounds_.min.z },
			Vector3{ bounds_.max.x, bounds_.min.y, bounds_.max.z },
			Vector3{ bounds_.min.x, bounds_.min.y, bounds_.max.z },
			Vector3{ bounds_.min.x, bounds_.max.y, bounds_.min.z },
			Vector3{ bounds_.max.x, bounds_.max.y, bounds_.min.z },
			Vector3{ bounds_.max.x, bounds_.max.y, bounds_.max.z },
			Vector3{ bounds_.min.x, bounds_.max.y, bounds_.max.z }
		};

		const std::array<std::array<int, 2>, 12> edges = {
			std::array<int, 2>{ 0, 1 },
			std::array<int, 2>{ 1, 2 },
			std::array<int, 2>{ 2, 3 },
			std::array<int, 2>{ 3, 0 },
			std::array<int, 2>{ 4, 5 },
			std::array<int, 2>{ 5, 6 },
			std::array<int, 2>{ 6, 7 },
			std::array<int, 2>{ 7, 4 },
			std::array<int, 2>{ 0, 4 },
			std::array<int, 2>{ 1, 5 },
			std::array<int, 2>{ 2, 6 },
			std::array<int, 2>{ 3, 7 }
		};

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

				screenPosition.x =
					(ndcX + 1.0f) * 0.5f * screenWidth;
				screenPosition.y =
					(1.0f - ndcY) * 0.5f * screenHeight;
				return true;
			};

		const ImU32 drawColor = IM_COL32(
			static_cast<int>(color.x * 255.0f),
			static_cast<int>(color.y * 255.0f),
			static_cast<int>(color.z * 255.0f),
			static_cast<int>(color.w * 255.0f)
		);

		ImDrawList* drawList = ImGui::GetForegroundDrawList();

		for (const auto& edge : edges) {
			ImVec2 start;
			ImVec2 end;

			if (!worldToScreen(corners[edge[0]], start) ||
				!worldToScreen(corners[edge[1]], end)) {
				continue;
			}

			drawList->AddLine(start, end, drawColor, 2.0f);
		}
#else
		(void)viewProjectionMatrix;
		(void)screenWidth;
		(void)screenHeight;
		(void)color;
#endif
	}

private:
	void UpdateBounds()
	{
		bounds_.min = {
			center_.x - halfSize_.x,
			center_.y - halfSize_.y,
			center_.z - halfSize_.z
		};
		bounds_.max = {
			center_.x + halfSize_.x,
			center_.y + halfSize_.y,
			center_.z + halfSize_.z
		};
	}

	Vector3 center_{ 0.0f, 0.0f, 0.0f };
	Vector3 halfSize_{ 0.5f, 1.0f, 0.5f };
	Bounds bounds_{};

	uint32_t attribute_ = 0;
	uint32_t mask_ = 0;
};
