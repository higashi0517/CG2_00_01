#pragma once
#include <cstdint>
#include <Windows.h>
#include "imgui/imgui.h"
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

class WinApp
{
public: // 静的メンバ変数
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam);

	static const int32_t kClientWidth = 1280;
	static const int32_t kClientHeight = 720;

public:
	// 初期化
	void Initialize();
	// 更新
	void Update();
	// 終了
	void Finalize();

	// getter
	HWND GetHwnd() const { return hwnd; }
	HINSTANCE GetHinstance() const { return wc.hInstance; }

	// メッセージの処理
	bool ProcessMessage();

private:
	// ウィンドウハンドル
	HWND hwnd = nullptr;

	// ウィンドウクラスの設定
	WNDCLASS wc = {};
};

