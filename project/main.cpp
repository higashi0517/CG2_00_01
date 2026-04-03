//#include "ResourceObject.h"
#include <Windows.h>
#include <wrl.h>
#include <filesystem>
#include <fstream>
#include <chrono>
//#include <d3d12.h>
//#include <dxgi1_6.h>
#include <cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#include <string>
#include <dbghelp.h>
#pragma comment(lib,"Dbghelp.lib")
#include <strsafe.h>
#include <dxgidebug.h>
#pragma comment(lib,"dxguid.lib")
#include <dxcapi.h>
#pragma comment(lib,"dxcompiler.lib")
#include "Matrix4x4.h"
#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx12.h"
#include "externals/DirectXTex/DirectXTex.h"
#include "externals/DirectXTex/d3dx12.h"
#include <vector>
#include <sstream>
#include <xaudio2.h>
#pragma comment (lib,"xaudio2.lib")
#include "Logger.h"
#include "StringUtility.h"
#include "D3DResourceLeakChecker.h"
#include "Game.h"
#include "Framework.h"

// クライアント領域のサイズ
//using float32_t = float;

using namespace Logger;
using namespace StringUtility;

static LONG WINAPI ExportDump(EXCEPTION_POINTERS* exception) {

	// 時間を取得して、時刻を名前の入れたファイルを作成
	SYSTEMTIME time;
	GetLocalTime(&time);
	wchar_t filePath[MAX_PATH] = { 0 };
	CreateDirectory(L"./Dump", NULL);
	StringCchPrintfW(filePath, MAX_PATH, L"./Dump/%04d-%02d%02d-%02d%02d.dmp", time.wYear, time.wMonth, time.wDay, time.wHour, time.wMinute);
	HANDLE dumpFileHandle = CreateFile(filePath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_WRITE | FILE_SHARE_READ, 0, CREATE_ALWAYS, 0, 0);

	// processIdとクラッシュの発生したthreadIdを取得
	DWORD processId = GetCurrentProcessId();
	DWORD threadId = GetCurrentThreadId();

	// 設定情報を入力
	MINIDUMP_EXCEPTION_INFORMATION minidumpInformation{ 0 };
	minidumpInformation.ThreadId = threadId;
	minidumpInformation.ExceptionPointers = exception;
	minidumpInformation.ClientPointers = TRUE;

	// Dumpを出力
	MiniDumpWriteDump(GetCurrentProcess(), processId, dumpFileHandle, MiniDumpNormal, &minidumpInformation, nullptr, nullptr);

	return EXCEPTION_EXECUTE_HANDLER;
}

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	CoInitializeEx(0, COINIT_MULTITHREADED);

	// リソースリークチェック
	D3DResourceLeakChecker* leakChecker = nullptr;

	SetUnhandledExceptionFilter(ExportDump);

	Framework* game = new Game();
	game->Run();	

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
	Log("ログの書き込み");

	// 初期化完了のログ
	Log("Complete create D3D12Device!!!\n");

	delete game;

	return 0;
}