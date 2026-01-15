#pragma once
#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <cstdint>
#include <array>
#include <string>
#include <dxcapi.h>
#include <externals/DirectXTex/DirectXTex.h>
#include <chrono>

class WinApp;

class GraphicsDevice
{

public:
	// 初期化
	void Initialize(WinApp* winApp);
	// 描画前処理
	void PreDraw();
	// 描画後処理
	void PostDraw();

	// SRVの指定番号のでスクリプタハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetSRVCPUDescriptorHandle(uint32_t index);  // CPU
	D3D12_GPU_DESCRIPTOR_HANDLE GetSRVGPUDescriptorHandle(uint32_t index);  // GPU

	// デバイスの取得
	Microsoft::WRL::ComPtr<ID3D12Device> GetDevice() { return device; }
	// DXGIファクトリーの取得
	Microsoft::WRL::ComPtr<IDXGIFactory7> GetDxgiFactory() { return dxgiFactory; }
	// コマンドリストの取得
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> GetCommandList() { return commandList; }

	// シェーダーコンパイル
	Microsoft::WRL::ComPtr<IDxcBlob> CompileShader(const std::wstring& filePath, const wchar_t* profile);

	// バッファリソース生成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateBufferResource(size_t sizeInBytes);

	// テクスチャリソース生成
	Microsoft::WRL::ComPtr<ID3D12Resource> CreateTextureResource(const DirectX::TexMetadata& metadata);

	// テクスチャアップロード
	Microsoft::WRL::ComPtr<ID3D12Resource> UploadTextureData(const DirectX::ScratchImage& mipImages, const Microsoft::WRL::ComPtr<ID3D12Resource>& texture);

	// 最大SRV数
	static const uint32_t kMaxSRVCount;

	// コマンドリスト関連
	void CloseCommandList();
	void ExecuteCommandList();
	void WaitForGPU();
	void ResetCommandList();

private:
	// デバイスの初期化
	void Device();
	// コマンドキューの初期化
	void CommandQueue();
	// スワップチェーンの初期化
	void SwapChain();
	// 深度バッファの初期化
	void DepthBuffer(int32_t width, int32_t height);
	// デスクリプターヒープの初期化
	void DescriptorHeap();
	// デスクリプターヒープの生成
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(D3D12_DESCRIPTOR_HEAP_TYPE heapType, UINT numDescriptors, bool shaderVisible);
	// レンダーターゲットビューの初期化
	void RenderTargetView();
	// 深度ステンシルビューの初期化
	void DepthStencilView();
	// フェンスの初期化
	void Fence();
	// ビューポート矩形の初期化
	void ViewportRect();
	// シザー矩形の初期化
	void ScissorRect();
	// DXCコンパイラの生成
	void DxcCompiler();
	// ImGuiの初期化
	void InitializeImGui();
	// FPS固定初期化
	void InitializeFixFPS();
	// FPS固定処理
	void UpdateFixFPS();

	// デバイス
	Microsoft::WRL::ComPtr<ID3D12Device> device;
	// DXGIファクトリー
	Microsoft::WRL::ComPtr<IDXGIFactory7> dxgiFactory;
	// コマンドキュー
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> commandQueue;
	// コマンドアロケータ
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> commandAllocator;
	// コマンドリスト
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> commandList;
	// デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap;
	// スワップチェーン
	Microsoft::WRL::ComPtr<IDXGISwapChain4> swapChain;
	// デスクリプタハンドルの取得
	D3D12_CPU_DESCRIPTOR_HANDLE GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);  // CPU
	D3D12_GPU_DESCRIPTOR_HANDLE GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> descriptorHeap, uint32_t descriptorSize, uint32_t index);  // GPU
	// スワップチェーンリソース
	std::array<Microsoft::WRL::ComPtr<ID3D12Resource>, 2> swapChainResources;
	// スワップチェーン設定
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	// RTVディスクリプタ
	D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
	// デスクリプタヒープ
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> rtvDescriptorHeap; // RTV用
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvDescriptorHeap; // SRV用
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> dsvDescriptorHeap; // DSV用
	// デスクリプタサイズ
	uint32_t descriptorSizeSRV = 0;
	uint32_t descriptorSizeRTV = 0;
	uint32_t descriptorSizeDSV = 0;
	//	デプスステンシルリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilReosurce;
	// フェンス
	Microsoft::WRL::ComPtr<ID3D12Fence> fence;
	// ビューポート
	D3D12_VIEWPORT viewport{};
	// rtvハンドル
	D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle[2];
	// シザー矩形
	D3D12_RECT scissorRect{};
	// フェンス値
	uint64_t fenceValue = 0;
	// フェンスイベント
	HANDLE fenceEvent;
	// DXCコンパイラ
	Microsoft::WRL::ComPtr<IDxcUtils> dxcUtils;
	Microsoft::WRL::ComPtr<IDxcCompiler3> dxcCompiler;
	Microsoft::WRL::ComPtr<IDxcIncludeHandler> includeHandler;
	// 記録時間
	std::chrono::steady_clock::time_point reference_;

	// WindowsAPI
	WinApp* winApp_ = nullptr;
};

