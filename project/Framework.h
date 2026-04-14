#pragma once
#include "BaseScene.h"

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

protected:
	bool endRequst_ = false;
	BaseScene* scene_ = nullptr;
};

