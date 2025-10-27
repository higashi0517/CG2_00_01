#pragma once
#include "Matrix4x4.h"
#include <dinput.h>


class DebugCamera
{

public:
	void Initialize();
	void Update(const BYTE key[256]);
	const Matrix4x4& GetViewMatrix() const { return viewMatrix_; }

private:
	Vector3 rotation_ = { 0, 0, 0 };
	Vector3 translation_ = { 0, 0, -50 };
	Matrix4x4 viewMatrix_;
};

