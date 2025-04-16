#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <chrono>

void Log(std::ostream& os, const std::string& message) {

	os << message << std::endl;

	OutputDebugStringA(message.c_str());
}

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// ログのディレクトリを表示
	std::filesystem::create_directory("logs");

	// 現在時間を取得
	std::chrono::system_clock::time_point now = std::chrono::system_clock::now();

	// ログファイルを秒に
	std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>
		nowSeconds = std::chrono::time_point_cast<std::chrono::seconds>(now);

	// 日本時間に変更
	std::chrono::zoned_time localTime{ std::chrono::current_zone(),nowSeconds };

	// formatを使って年月日_時分秒の文字列に変換
	std::string dataString = std::format("{:%y%m%d_%H%M%S}", localTime);

	// 時刻を使ってファイル名の決定
	std::string logFilePath = std::string("logs/") + dataString + ".log";

	// ファイルを作って書き込み準備
	std::ofstream logStream(logFilePath);

	// 書き込み
	Log(logStream, "ログの書き込み");

	return 0;
}