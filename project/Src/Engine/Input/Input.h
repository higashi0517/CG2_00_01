#pragma once
#include <Windows.h>
#include <wrl.h>
#include <dinput.h>
#include "WinApp.h"

class Input
{
public:
	template<class T> using ComPtr = Microsoft::WRL::ComPtr<T>;

public:

	// 初期化
	void Initialize(WinApp* winApp);
	// 更新
	void Update();

	/// <summary>
	/// キーの押下判定
	/// </summary>
	/// <param name="keyNumber">DIK_***の定数</param>
	bool PushKey(BYTE keyNumber);

	/// <summary>
	/// キーのトリガー判定
	/// </summary>
	/// 	<param name="keyNumber">DIK_***の定数</param>
	bool TriggerKey(BYTE keyNumber);
private:

	ComPtr<IDirectInputDevice8> keyboard = nullptr;

	// 全キーの入力状態
	BYTE key[256] = {};

	// 前回フレームの全キーの入力状態
	BYTE keyPre[256] = {};

	ComPtr <IDirectInput8> directInput;

	// WindowsAPI
	WinApp* winApp_ = nullptr;
};

