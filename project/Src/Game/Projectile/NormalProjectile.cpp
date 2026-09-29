#include "NormalProjectile.h"

void NormalProjectile::Initialize(
	SpriteManager* spriteManager,
	const Vector3& position,
	const Vector3& velocity)
{
	Projectile::Initialize(
		spriteManager,
		"Resources/circle2.png",
		ProjectileOwner::Player,
		position,
		velocity,
		{ 0.1f, 0.6f, 1.0f, 1.0f });
}
