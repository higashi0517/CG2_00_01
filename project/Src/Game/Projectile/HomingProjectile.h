#pragma once

#include "Projectile.h"

class Enemy;
class EnemySpawnController;
class SpriteManager;

// 発射時に指定された敵を追尾するミサイル
class HomingProjectile final : public Projectile
{
public:
	void Initialize(
		SpriteManager* spriteManager,
		EnemySpawnController* enemySpawnController,
		Enemy* target,
		const Vector3& position,
		const Vector3& initialVelocity);
	void Update(const Camera& camera) override;

private:
	EnemySpawnController* enemySpawnController_ = nullptr;
	Enemy* target_ = nullptr;
	float speed_ = 0.13f;
	float homingRate_ = 0.08f;
};
