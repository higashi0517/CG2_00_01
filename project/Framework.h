#pragma once
#include "SceneManager.h"
#include "AbstractSceneFactory.h"

class Framework
{
public:

	virtual void Initialize();
	virtual void Update();
	virtual void Draw() = 0;
	virtual void Finalize();
	virtual bool IsEndRequst() { return endRequst_; }
	virtual ~Framework() = default;
	void Run();

private:
	bool endRequst_ = false;
	AbstractSceneFactory* sceneFactory_ = nullptr;
};

