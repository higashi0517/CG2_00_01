#pragma once
#include <Windows.h>
#include "GraphicsDevice.h" 
#include "WinApp.h"
#include "imgui/imgui_impl_win32.h"
#include "imgui/imgui_impl_dx12.h"

class ImGuiManager
{
public:
	// シングルトンインスタンスの取得
	static ImGuiManager* GetInstance();

	// 初期化
	void Initialize(WinApp* winApp, GraphicsDevice* graphicsDevice);

	// 終了処理
	void Finalize();

	// フレーム開始
	void Begin();

	// フレーム終了
	void End();

	// 描画
	void Draw();

private: // シングルトン用のコンストラクタ隠蔽
	ImGuiManager() = default;
	~ImGuiManager() = default;
	ImGuiManager(const ImGuiManager&) = delete;
	const ImGuiManager& operator=(const ImGuiManager&) = delete;

private:
	// DirectX基盤への参照
	GraphicsDevice* graphicsDevice_ = nullptr;

	// SRV関連のデータ（SrvManager導入後はそちらで管理するIDなどになります）
	// 現時点では初期化時に渡されるか、確保した場所を保持する必要があります
};