#include <Windows.h>
#include <cstdint>

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