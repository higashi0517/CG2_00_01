#include "Player.h"

#include "Input.h"
#include "Object3DManager.h"
#include "Skeleton.h"
#include <cassert>
#include <algorithm>
#include <cmath>

void Player::Initialize(Object3DManager* object3DManager, const std::string& modelFileName)
{
	assert(object3DManager != nullptr);

	object3D_ = std::make_unique<Object3D>();
	object3D_->Initialize(object3DManager);
	object3D_->SetModel(modelFileName);
	baseRotation_ = object3D_->GetRotate();
	movementTilt_ = {};

	collider_.SetHalfSize({ 0.35f, 1.0f, 0.35f });

	UpdateCollider();
	collider_.SetAttribute(CollisionLayer::Player);
	collider_.SetMask(
		CollisionLayer::Enemy |
		CollisionLayer::EnemyBullet
	);
}

void Player::Update(Input* input)
{
	assert(input != nullptr);

	Vector3 position = object3D_->GetTranslate();
	constexpr float kMoveSpeed = 0.1f;

	if (input->PushKey(DIK_W)) {
		position.z += kMoveSpeed;
	}
	if (input->PushKey(DIK_S)) {
		position.z -= kMoveSpeed;
	}
	if (input->PushKey(DIK_A)) {
		position.x -= kMoveSpeed;
	}
	if (input->PushKey(DIK_D)) {
		position.x += kMoveSpeed;
	}

	object3D_->SetTranslate(position);
	UpdateMovementTilt(
		float(input->PushKey(DIK_D)) - float(input->PushKey(DIK_A)),
		float(input->PushKey(DIK_W)) - float(input->PushKey(DIK_S)));
	UpdateTransform();
}

void Player::UpdateMovementTilt(float horizontal, float vertical)
{
	constexpr float kMaxRoll = 0.4f;
	constexpr float kMaxPitch = 0.2f;
	constexpr float kTiltFollow = 0.15f;
	const float targetPitch = -std::clamp(vertical, -1.0f, 1.0f) * kMaxPitch;
	const float targetRoll = -std::clamp(horizontal, -1.0f, 1.0f) * kMaxRoll;
	movementTilt_.x += (targetPitch - movementTilt_.x) * kTiltFollow;
	movementTilt_.z += (targetRoll - movementTilt_.z) * kTiltFollow;
	ApplyRotation();
}

void Player::ResetMovementTilt()
{
	movementTilt_ = {};
	ApplyRotation();
}

void Player::ApplyRotation()
{
	object3D_->SetRotate({
		baseRotation_.x + movementTilt_.x,
		baseRotation_.y,
		baseRotation_.z + movementTilt_.z
	});
	UpdateCollider();
}

void Player::UpdateTransform()
{
	object3D_->Update();
	UpdateCollider();
}

void Player::Draw(const SkinCluster& skinCluster)
{
	object3D_->Draw(skinCluster);
}

void Player::SetTranslate(const Vector3& translate)
{
	object3D_->SetTranslate(translate);

	UpdateCollider();
}

void Player::SetRotate(const Vector3& rotate)
{
	baseRotation_ = rotate;
	ApplyRotation();
}

void Player::SetScale(const Vector3& scale)
{
	object3D_->SetScale(scale);
	UpdateCollider();
}

const Vector3& Player::GetTranslate() const
{
	return object3D_->GetTranslate();
}

const Vector3& Player::GetRotate() const
{
	// 移動演出の傾きを含めず、進行方向・編集用の基本姿勢を返す。
	return baseRotation_;
}

const Vector3& Player::GetScale() const
{
	return object3D_->GetScale();
}

void Player::UpdateCollider()
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
