#include "HomingProjectile.h"

#include "Enemy.h"
#include "EnemySpawnController.h"
#include "SpriteManager.h"
#include <cmath>

void HomingProjectile::Initialize(
	SpriteManager* spriteManager,
	EnemySpawnController* enemySpawnController,
	Enemy* target,
	const Vector3& position,
	const Vector3& initialVelocity)
{
	enemySpawnController_ = enemySpawnController;
	target_ = target;

	const float initialSpeed = std::sqrt(
		initialVelocity.x * initialVelocity.x +
		initialVelocity.y * initialVelocity.y +
		initialVelocity.z * initialVelocity.z);
	if (initialSpeed > 0.0001f) {
		speed_ = initialSpeed;
	}

	Projectile::Initialize(
		spriteManager,
		"Resources/white1x1.png",
		ProjectileOwner::Player,
		position,
		initialVelocity,
		{ 1.0f, 0.45f, 0.05f, 1.0f });

	sprite_->SetSize({ 32.0f, 10.0f });
	sprite_->Update();
}

void HomingProjectile::Update(const Camera& camera)
{
	if (!isActive_) {
		return;
	}

	if (target_ != nullptr &&
		enemySpawnController_ != nullptr &&
		enemySpawnController_->ContainsEnemy(target_)) {
		Vector3 targetPosition = target_->GetTranslate();
		// 機体の中心へ追尾する。
		const Vector3 toTarget = {
			targetPosition.x - position_.x,
			targetPosition.y - position_.y,
			targetPosition.z - position_.z
		};
		const float targetLength = std::sqrt(
			toTarget.x * toTarget.x +
			toTarget.y * toTarget.y +
			toTarget.z * toTarget.z);

		if (targetLength > 0.0001f) {
			const Vector3 desiredDirection = {
				toTarget.x / targetLength,
				toTarget.y / targetLength,
				toTarget.z / targetLength
			};
			const float currentLength = std::sqrt(
				velocity_.x * velocity_.x +
				velocity_.y * velocity_.y +
				velocity_.z * velocity_.z);
			Vector3 currentDirection = desiredDirection;
			if (currentLength > 0.0001f) {
				currentDirection = {
					velocity_.x / currentLength,
					velocity_.y / currentLength,
					velocity_.z / currentLength
				};
			}

			const Vector3 blendedDirection = {
				currentDirection.x * (1.0f - homingRate_) +
					desiredDirection.x * homingRate_,
				currentDirection.y * (1.0f - homingRate_) +
					desiredDirection.y * homingRate_,
				currentDirection.z * (1.0f - homingRate_) +
					desiredDirection.z * homingRate_
			};
			const float blendedLength = std::sqrt(
				blendedDirection.x * blendedDirection.x +
				blendedDirection.y * blendedDirection.y +
				blendedDirection.z * blendedDirection.z);
			if (blendedLength > 0.0001f) {
				velocity_ = {
					blendedDirection.x / blendedLength * speed_,
					blendedDirection.y / blendedLength * speed_,
					blendedDirection.z / blendedLength * speed_
				};
			}
		}
	}
	else {
		target_ = nullptr;
	}

	Projectile::Update(camera);
	if (!isActive_ || !isVisible_) {
		return;
	}

	Vector2 currentScreen{};
	Vector2 nextScreen{};
	float currentClipW = 0.0f;
	float nextClipW = 0.0f;
	const Vector3 nextPosition = {
		position_.x + velocity_.x * 8.0f,
		position_.y + velocity_.y * 8.0f,
		position_.z + velocity_.z * 8.0f
	};
	if (WorldPositionToScreen(
		camera, position_, currentScreen, currentClipW) &&
		WorldPositionToScreen(
			camera, nextPosition, nextScreen, nextClipW)) {
		const float angle = std::atan2(
			nextScreen.y - currentScreen.y,
			nextScreen.x - currentScreen.x);
		float size = 180.0f / currentClipW;
		if (size < 10.0f) {
			size = 10.0f;
		}
		if (size > 34.0f) {
			size = 34.0f;
		}
		sprite_->SetRotation(angle);
		sprite_->SetSize({ size * 1.8f, size * 0.65f });
		sprite_->Update();
	}
}
