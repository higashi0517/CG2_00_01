#pragma once
#include <GraphicsDevice.h>

class ModelCommon
{
private:
	GraphicsDevice* graphicsDevice_;

public:
	// 初期化
	void Initialize(GraphicsDevice* graphicsDevice);

	GraphicsDevice* GetGraphicsDevice() const { return graphicsDevice_; }
};

