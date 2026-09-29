#pragma once

#include "Camera.h"
#include "Collider.h"
#include "Sprite.h"
#include <memory>
#include <string>

class SpriteManager;

enum class ProjectileOwner
{
	Player,
	Enemy
};

// ワールド座標で移動し、画面上ではカメラ正面を向く弾
class Projectile
{
public:
	virtual ~Projectile() = default;

	void Initialize(
		SpriteManager* spriteManager,
		const std::string& textureFilePath,
		ProjectileOwner owner,
		const Vector3& position,
		const Vector3& velocity,
		const Vector4& color
	);

	virtual void Update(const Camera& camera);
	virtual void Draw();

	void Deactivate() { isActive_ = false; }
	bool IsActive() const { return isActive_; }
	ProjectileOwner GetOwner() const { return owner_; }
	const Collider& GetCollider() const { return collider_; }
	const Vector3& GetPosition() const { return position_; }

protected:
	bool WorldToScreen(
		const Camera& camera,
		Vector2& screenPosition,
		float& clipW
	) const;
	bool WorldPositionToScreen(
		const Camera& camera,
		const Vector3& worldPosition,
		Vector2& screenPosition,
		float& clipW
	) const;

	std::unique_ptr<Sprite> sprite_;
	ProjectileOwner owner_ = ProjectileOwner::Player;
	Vector3 position_{ 0.0f, 0.0f, 0.0f };
	Vector3 velocity_{ 0.0f, 0.0f, 0.0f };
	Collider collider_;
	int remainingFrames_ = 300;
	bool isActive_ = true;
	bool isVisible_ = false;
};
