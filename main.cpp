#include <Windows.h>
#include <filesystem>
#include <fstream>
#include <chrono>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <cassert>
#pragma comment(lib,"d3d12.lib")
#pragma comment(lib,"dxgi.lib")
#include <string>

// クライアント領域のサイズ
const int32_t kClientWindth = 1280;
const int32_t kClientHeight = 720;

// ウィンドウプロシージャ
LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {

	// メッセージ処理
	switch (msg) {
		// ウィンドウが閉じられた
	case WM_DESTROY:
		// アプリの終了
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, msg, wparam, lparam);
}

void Log(std::ostream& os, const std::string& message) {

	os << message << std::endl;

	OutputDebugStringA(message.c_str());
}

std::wstring ConvertString(const std::string& str) {
	if (str.empty()) {
		return std::wstring();
	}

	auto sizeNeeded = MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), NULL, 0);
	if (sizeNeeded == 0) {
		return std::wstring();
	}
	std::wstring result(sizeNeeded, 0);
	MultiByteToWideChar(CP_UTF8, 0, reinterpret_cast<const char*>(&str[0]), static_cast<int>(str.size()), &result[0], sizeNeeded);
	return result;
}

std::string ConvertString(const std::wstring& str) {
	if (str.empty()) {
		return std::string();
	}

	auto sizeNeeded = WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), NULL, 0, NULL, NULL);
	if (sizeNeeded == 0) {
		return std::string();
	}
	std::string result(sizeNeeded, 0);
	WideCharToMultiByte(CP_UTF8, 0, str.data(), static_cast<int>(str.size()), result.data(), sizeNeeded, NULL, NULL);
	return result;
}

// Windowsアプリでのエントリーポイント
int WINAPI WinMain(HINSTANCE, HINSTANCE, LPSTR, int) {

	// ウィンドウクラス
	WNDCLASS wc = {};

	// ウィンドウプロシージャ
	wc.lpfnWndProc = WindowProc;
	// ウィンドウクラス名
	wc.lpszClassName = L"CG2WondowClass";
	// インスタンスハンドル
	wc.hInstance = GetModuleHandle(nullptr);
	// カーソル
	wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

	// ウィンドウクラスの登録
	RegisterClass(&wc);

	// ウィンドウサイズを表す構造体にクライアント領域を入れる
	RECT wrc = { 0,0,kClientWindth,kClientHeight };

	// クライアント領域を元に実際のサイズにwrcを変更してもらう
	AdjustWindowRect(&wrc, WS_OVERLAPPEDWINDOW, false);

	// ウィンドウの生成
	HWND hwnd = CreateWindow(
		wc.lpszClassName,          // クラス名
		L"CG2",                    // タイトルバーのテキスト
		WS_OVERLAPPEDWINDOW,       // ウィンドウスタイル
		CW_USEDEFAULT,             // 表示X座標
		CW_USEDEFAULT,             // 表示Y座標
		wrc.right - wrc.left,      // ウィンドウの横幅
		wrc.bottom - wrc.top,      // ウィンドウの縦幅
		nullptr,                   // 親ウィンドウハンドル
		nullptr,                   // メニューハンドル
		wc.hInstance,              // インスタンスハンドル
		nullptr                    // オプション
	);

	// ウィンドウの表示
	ShowWindow(hwnd, SW_SHOW);


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

	// DXGIファクトリーの生成
	IDXGIFactory7* dxgiFactory = nullptr;
	HRESULT hr = CreateDXGIFactory(IID_PPV_ARGS(&dxgiFactory));
	assert(SUCCEEDED(hr));

	// アダプタの変数
	IDXGIAdapter4* useAdapter = nullptr;

	// アダプタの列挙
	for (UINT i = 0; dxgiFactory->EnumAdapterByGpuPreference(
		i, DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE, IID_PPV_ARGS(&useAdapter)) != DXGI_ERROR_NOT_FOUND; ++i) {

		// アダプタの情報を取得
		DXGI_ADAPTER_DESC3 adapterDesc;
		hr = useAdapter->GetDesc3(&adapterDesc);
		assert(SUCCEEDED(hr));

		if (!(adapterDesc.Flags & DXGI_ADAPTER_FLAG3_SOFTWARE)) {

			// アダプタの情報をログに出力
			Log(logStream,ConvertString(std::format(L"Use Adapter {}: \n", adapterDesc.Description)));
			break;
		}
		useAdapter = nullptr;
	}
	assert(useAdapter != nullptr);

	ID3D12Device* device = nullptr;
	D3D_FEATURE_LEVEL featureLevels[] = {
		D3D_FEATURE_LEVEL_12_2,
		D3D_FEATURE_LEVEL_12_1,
		D3D_FEATURE_LEVEL_12_0
	};

	const char* featureLevelStrings[] = {
		"12.2",
		"12.1",
		"12.0"
	};

	for (size_t i = 0; i < _countof(featureLevels); i++) {
		// デバイスの生成
		hr = D3D12CreateDevice(useAdapter, featureLevels[i], IID_PPV_ARGS(&device));
		if (SUCCEEDED(hr)) {

			Log(logStream,std::format("Feature Level: {}\n", featureLevelStrings[i]));
			break;
		}
	}
	assert(device != nullptr);
	// 初期化完了のログ
	Log(logStream,"Complete create D3D12Device!!!\n");

	MSG msg = {};

	// ウィンドウのボタンが押されるまでループ
	while (msg.message != WM_QUIT) {

		// メッセージがあれば処理
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE)) {

			TranslateMessage(&msg);
			DispatchMessage(&msg);
		} else {

			//ゲーム処理
		}
	}

	return 0;
}

