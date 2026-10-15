#pragma once

#include "Object3D.h"
#include "Collider.h"
#include <memory>
#include <string>

class Object3DManager;
struct SkinCluster;

class Enemy
{
public:
	void Initialize(Object3DManager* object3DManager, const std::string& modelFileName);
	void Update();
	void Update(const Vector3& playerPosition);
	void Draw(const SkinCluster& skinCluster);
	void StartEntrance(const Vector3& start, const Vector3& destination, int durationFrames);
	bool IsEntering() const { return entranceFrame_ < entranceDuration_; }

	void SetTranslate(const Vector3& translate);
	void SetRotate(const Vector3& rotate);
	void SetScale(const Vector3& scale);

	const Vector3& GetTranslate() const;
	const Vector3& GetRotate() const;
	const Vector3& GetScale() const;
	const Collider& GetCollider() const { return collider_; }

private:
	std::unique_ptr<Object3D> object3D_;
	Collider collider_;
	Vector3 entranceStart_{};
	Vector3 entranceDestination_{};
	int entranceFrame_ = 0;
	int entranceDuration_ = 0;
	void UpdateCollider();
};
