#pragma once
class WinApp;
class GraphicsDevice;

class BaseScene
{
public:

	virtual ~BaseScene() = default;
	virtual void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice) = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void Finalize() = 0;
};

