#include "EnemySpawnController.h"

#include "Collider.h"
#include <algorithm>
#include <cassert>
#include <cfloat>

void EnemySpawnController::Initialize(
	Object3DManager* object3DManager,
	const std::string& modelFileName)
{
	assert(object3DManager != nullptr);
	object3DManager_ = object3DManager;
	modelFileName_ = modelFileName;
	Reset();
}

void EnemySpawnController::AddSpawnEvent(
	int frame,
	const Vector3& position)
{
	const int spawnFrame = frame > 0 ? frame : 0;
	spawnEvents_.push_back({ spawnFrame, position });
	std::sort(
		spawnEvents_.begin(),
		spawnEvents_.end(),
		[](const SpawnEvent& lhs, const SpawnEvent& rhs) {
			return lhs.frame < rhs.frame;
		}
	);
}

void EnemySpawnController::Update(const Vector3& playerPosition)
{
	while (nextSpawnIndex_ < spawnEvents_.size() &&
		spawnEvents_[nextSpawnIndex_].frame <= elapsedFrames_) {
		Spawn(spawnEvents_[nextSpawnIndex_].position);
		++nextSpawnIndex_;
	}

	for (auto& enemy : enemies_) {
		enemy->Update(playerPosition);
	}

	const float despawnDistanceSquared =
		despawnDistance_ * despawnDistance_;
	enemies_.erase(
		std::remove_if(
			enemies_.begin(),
			enemies_.end(),
			[&](const std::unique_ptr<Enemy>& enemy) {
				const Vector3 enemyPosition = enemy->GetTranslate();
				const float dx = enemyPosition.x - playerPosition.x;
				const float dy = enemyPosition.y - playerPosition.y;
				const float dz = enemyPosition.z - playerPosition.z;
				return dx * dx + dy * dy + dz * dz >
					despawnDistanceSquared;
			}
		),
		enemies_.end()
	);

	++elapsedFrames_;
}

void EnemySpawnController::Draw(const SkinCluster& skinCluster)
{
	for (auto& enemy : enemies_) {
		enemy->Draw(skinCluster);
	}
}

void EnemySpawnController::Reset()
{
	enemies_.clear();
	nextSpawnIndex_ = 0;
	elapsedFrames_ = 0;
}

Enemy* EnemySpawnController::GetNearestEnemy(
	const Vector3& position) const
{
	Enemy* nearestEnemy = nullptr;
	float nearestDistanceSquared = FLT_MAX;

	for (const auto& enemy : enemies_) {
		const Vector3 enemyPosition = enemy->GetTranslate();
		const float dx = enemyPosition.x - position.x;
		const float dy = enemyPosition.y - position.y;
		const float dz = enemyPosition.z - position.z;
		const float distanceSquared = dx * dx + dy * dy + dz * dz;

		if (distanceSquared < nearestDistanceSquared) {
			nearestDistanceSquared = distanceSquared;
			nearestEnemy = enemy.get();
		}
	}

	return nearestEnemy;
}

Enemy* EnemySpawnController::GetLockOnTarget(
	const Camera& camera,
	const Vector2& reticlePosition,
	float lockOnRadius) const
{
	constexpr float kScreenWidth = 1280.0f;
	constexpr float kScreenHeight = 720.0f;
	const Matrix4x4& viewProjection = camera.GetViewProjectionMatrix();
	const float lockOnRadiusSquared = lockOnRadius * lockOnRadius;
	float nearestScreenDistanceSquared = FLT_MAX;
	Enemy* lockOnTarget = nullptr;

	for (const auto& enemy : enemies_) {
		Vector3 position = enemy->GetTranslate();
		// 機体の原点を照準の対象にする。
		const float clipX =
			position.x * viewProjection.m[0][0] +
			position.y * viewProjection.m[1][0] +
			position.z * viewProjection.m[2][0] +
			viewProjection.m[3][0];
		const float clipY =
			position.x * viewProjection.m[0][1] +
			position.y * viewProjection.m[1][1] +
			position.z * viewProjection.m[2][1] +
			viewProjection.m[3][1];
		const float clipZ =
			position.x * viewProjection.m[0][2] +
			position.y * viewProjection.m[1][2] +
			position.z * viewProjection.m[2][2] +
			viewProjection.m[3][2];
		const float clipW =
			position.x * viewProjection.m[0][3] +
			position.y * viewProjection.m[1][3] +
			position.z * viewProjection.m[2][3] +
			viewProjection.m[3][3];
		if (clipW <= 0.0f) {
			continue;
		}

		const float ndcZ = clipZ / clipW;
		if (ndcZ < 0.0f || ndcZ > 1.0f) {
			continue;
		}

		const Vector2 screenPosition = {
			(clipX / clipW + 1.0f) * 0.5f * kScreenWidth,
			(1.0f - clipY / clipW) * 0.5f * kScreenHeight
		};
		const float dx = screenPosition.x - reticlePosition.x;
		const float dy = screenPosition.y - reticlePosition.y;
		const float screenDistanceSquared = dx * dx + dy * dy;
		if (screenDistanceSquared <= lockOnRadiusSquared &&
			screenDistanceSquared < nearestScreenDistanceSquared) {
			nearestScreenDistanceSquared = screenDistanceSquared;
			lockOnTarget = enemy.get();
		}
	}

	return lockOnTarget;
}

bool EnemySpawnController::ContainsEnemy(const Enemy* target) const
{
	if (target == nullptr) {
		return false;
	}
	for (const auto& enemy : enemies_) {
		if (enemy.get() == target) {
			return true;
		}
	}
	return false;
}

bool EnemySpawnController::HitEnemy(const Collider& attackCollider)
{
	for (auto iterator = enemies_.begin(); iterator != enemies_.end(); ++iterator) {
		if (attackCollider.IsCollision((*iterator)->GetCollider())) {
			enemies_.erase(iterator);
			return true;
		}
	}
	return false;
}

bool EnemySpawnController::IsCollision(const Collider& collider) const
{
	for (const auto& enemy : enemies_) {
		if (collider.IsCollision(enemy->GetCollider())) {
			return true;
		}
	}
	return false;
}

void EnemySpawnController::Spawn(const Vector3& position)
{
	auto enemy = std::make_unique<Enemy>();
	enemy->Initialize(object3DManager_, modelFileName_);
	enemy->SetScale({ 0.75f, 0.75f, 0.75f });
	enemy->SetTranslate(position);
	enemies_.push_back(std::move(enemy));
}
