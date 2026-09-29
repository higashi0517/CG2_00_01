#pragma once

#include "Projectile.h"

class SpriteManager;

// ロックオンしていないときに直進する通常弾
class NormalProjectile final : public Projectile
{
public:
	void Initialize(
		SpriteManager* spriteManager,
		const Vector3& position,
		const Vector3& velocity);
};
