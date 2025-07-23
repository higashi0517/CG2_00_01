#pragma once
#include "Matrix4x4.h"

class DebugCamera
{

public:
	void Initialize();
	void Update();

private:
	Vector3 rotation_ = { 0, 0, 0 };
	Vector3 translation_ = { 0, 0, -50 };
	
};

