#include "Projectile.h"

#include "SpriteManager.h"
#include <algorithm>

namespace
{
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;
	constexpr float kProjectileRadius = 0.18f;
}

void Projectile::Initialize(
	SpriteManager* spriteManager,
	const std::string& textureFilePath,
	ProjectileOwner owner,
	const Vector3& position,
	const Vector3& velocity,
	const Vector4& color)
{
	owner_ = owner;
	position_ = position;
	velocity_ = velocity;
	remainingFrames_ = 300;
	isActive_ = true;
	isVisible_ = false;

	sprite_ = std::make_unique<Sprite>();
	sprite_->Initialize(spriteManager, textureFilePath);
	sprite_->SetAnchorPoint({ 0.5f, 0.5f });
	sprite_->SetColor(color);
	sprite_->SetSize({ 20.0f, 20.0f });
	sprite_->Update();

	collider_.SetCenter(position_);
	collider_.SetHalfSize({
		kProjectileRadius,
		kProjectileRadius,
		kProjectileRadius
		});

	if (owner_ == ProjectileOwner::Player) {
		collider_.SetAttribute(CollisionLayer::PlayerBullet);
		collider_.SetMask(
			CollisionLayer::Enemy |
			CollisionLayer::EnemyBullet
		);
	}
	else {
		collider_.SetAttribute(CollisionLayer::EnemyBullet);
		collider_.SetMask(
			CollisionLayer::Player |
			CollisionLayer::PlayerBullet
		);
	}
}

void Projectile::Update(const Camera& camera)
{
	if (!isActive_) {
		return;
	}

	position_.x += velocity_.x;
	position_.y += velocity_.y;
	position_.z += velocity_.z;
	collider_.SetCenter(position_);

	--remainingFrames_;
	if (remainingFrames_ <= 0) {
		isActive_ = false;
		isVisible_ = false;
		return;
	}

	Vector2 screenPosition{};
	float clipW = 0.0f;
	isVisible_ = WorldToScreen(camera, screenPosition, clipW);

	if (!isVisible_) {
		return;
	}

	const float screenSize = std::clamp(140.0f / clipW, 8.0f, 30.0f);
	sprite_->SetPosition(screenPosition);
	sprite_->SetSize({ screenSize, screenSize });
	sprite_->Update();
}

void Projectile::Draw()
{
	if (isActive_ && isVisible_ && sprite_) {
		sprite_->Draw();
	}
}

bool Projectile::WorldToScreen(
	const Camera& camera,
	Vector2& screenPosition,
	float& clipW) const
{
	return WorldPositionToScreen(
		camera,
		position_,
		screenPosition,
		clipW);
}

bool Projectile::WorldPositionToScreen(
	const Camera& camera,
	const Vector3& worldPosition,
	Vector2& screenPosition,
	float& clipW) const
{
	const Matrix4x4& viewProjection = camera.GetViewProjectionMatrix();

	const float clipX =
		worldPosition.x * viewProjection.m[0][0] +
		worldPosition.y * viewProjection.m[1][0] +
		worldPosition.z * viewProjection.m[2][0] +
		viewProjection.m[3][0];
	const float clipY =
		worldPosition.x * viewProjection.m[0][1] +
		worldPosition.y * viewProjection.m[1][1] +
		worldPosition.z * viewProjection.m[2][1] +
		viewProjection.m[3][1];
	const float clipZ =
		worldPosition.x * viewProjection.m[0][2] +
		worldPosition.y * viewProjection.m[1][2] +
		worldPosition.z * viewProjection.m[2][2] +
		viewProjection.m[3][2];
	clipW =
		worldPosition.x * viewProjection.m[0][3] +
		worldPosition.y * viewProjection.m[1][3] +
		worldPosition.z * viewProjection.m[2][3] +
		viewProjection.m[3][3];

	if (clipW <= 0.0f) {
		return false;
	}

	const float ndcX = clipX / clipW;
	const float ndcY = clipY / clipW;
	const float ndcZ = clipZ / clipW;

	if (ndcZ < 0.0f || ndcZ > 1.0f) {
		return false;
	}

	screenPosition.x = (ndcX + 1.0f) * 0.5f * kScreenWidth;
	screenPosition.y = (1.0f - ndcY) * 0.5f * kScreenHeight;
	return true;
}
