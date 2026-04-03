#pragma once
class Framework
{
protected:
	bool endRequst_ = false;
public:

	virtual void Initialize();
	virtual void Update();
	virtual void Draw() = 0;
	virtual void Finalize();
	virtual bool IsEndRequst() { return endRequst_; }
	virtual ~Framework() = default;
	void Run();
};

