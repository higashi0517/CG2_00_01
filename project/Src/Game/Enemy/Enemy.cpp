#include "Enemy.h"

#include "Object3DManager.h"
#include "Skeleton.h"
#include <cassert>
#include <cmath>

void Enemy::Initialize(Object3DManager* object3DManager, const std::string& modelFileName)
{
	assert(object3DManager != nullptr);

	object3D_ = std::make_unique<Object3D>();
	object3D_->Initialize(object3DManager);
	object3D_->SetModel(modelFileName);

	collider_.SetHalfSize({ 0.35f, 1.0f, 0.35f });

	UpdateCollider();
	collider_.SetAttribute(CollisionLayer::Enemy);
	collider_.SetMask(
		CollisionLayer::Player |
		CollisionLayer::PlayerBullet
	);
}

void Enemy::Update(const Vector3& playerPosition)
{
	const Vector3 enemyPosition = object3D_->GetTranslate();
	const float directionX = playerPosition.x - enemyPosition.x;
	const float directionZ = playerPosition.z - enemyPosition.z;

	if (directionX != 0.0f || directionZ != 0.0f) {
		Vector3 rotation = object3D_->GetRotate();
		rotation.y = std::atan2(directionX, directionZ);
		object3D_->SetRotate(rotation);
	}

	Update();
}

void Enemy::Update()
{
	object3D_->Update();
	UpdateCollider();
}

void Enemy::Draw(const SkinCluster& skinCluster)
{
	object3D_->Draw(skinCluster);
}

void Enemy::SetTranslate(const Vector3& translate)
{
	object3D_->SetTranslate(translate);

	UpdateCollider();
}

void Enemy::SetRotate(const Vector3& rotate)
{
	object3D_->SetRotate(rotate);
	UpdateCollider();
}

void Enemy::SetScale(const Vector3& scale)
{
	object3D_->SetScale(scale);
	UpdateCollider();
}

const Vector3& Enemy::GetTranslate() const
{
	return object3D_->GetTranslate();
}

const Vector3& Enemy::GetRotate() const
{
	return object3D_->GetRotate();
}

const Vector3& Enemy::GetScale() const
{
	return object3D_->GetScale();
}

void Enemy::UpdateCollider()
{
	// OBJの原点を中心に、機体全体を囲む箱を回転・拡縮する。
	const auto matrix = MakeAffineMatrix(object3D_->GetScale(), object3D_->GetRotate(), { 0.0f, 0.0f, 0.0f });
	constexpr Vector3 half = { 2.757f, 1.365f, 2.879f };
	collider_.SetHalfSize({
		std::abs(matrix.m[0][0]) * half.x + std::abs(matrix.m[1][0]) * half.y + std::abs(matrix.m[2][0]) * half.z,
		std::abs(matrix.m[0][1]) * half.x + std::abs(matrix.m[1][1]) * half.y + std::abs(matrix.m[2][1]) * half.z,
		std::abs(matrix.m[0][2]) * half.x + std::abs(matrix.m[1][2]) * half.y + std::abs(matrix.m[2][2]) * half.z
	});
	collider_.SetCenter(object3D_->GetTranslate());
}