#include "Framework.h"
#include "Logger.h"
#include <filesystem>
#include <fstream>
#include <chrono>

using namespace Logger;

void Framework::Run()
{
	// ゲームの初期化
	Initialize();

	while (true) 
	{
		// 毎フレーム更新
		Update();
		// 終了リクエストが来たら抜ける
		if (IsEndRequst()) {
			break;
		}
		// 描画
		Draw();
	}
	// ゲームの終了
	Finalize();
}

void Framework::Initialize() {
}

void Framework::Update() {
}

void Framework::Finalize() {

	std::filesystem::create_directory("logs");
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds> nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);
	std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };
	std::string dataString = std::format("{:%y%m%d_%H%M%S}", localTime);
	std::string logFilePath = std::string("logs/") + dataString + ".log";
	std::ofstream logStream(logFilePath);

	Log("ログの書き込み");
	Log("Complete create D3D12Device!!!\n");
}