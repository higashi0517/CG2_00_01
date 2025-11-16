#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

class GraphicsDevice
{

public:
	// 初期化
	void Initialize();
	// デバイスの初期化
	void Device();

	// デバイスの取得
	Microsoft::WRL::ComPtr<ID3D12Device> DeviceGet() { return device_; }
	// DXGIファクトリーの取得
	Microsoft::WRL::ComPtr<IDXGIFactory7> DxgiFactoryGet() { return dxgiFactory; }

private:
	// デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device_;
	// DXGIファクトリー
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
};

