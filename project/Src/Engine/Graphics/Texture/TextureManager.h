#pragma once
#include <string>
#include <DirectXTex/DirectXTex.h>
#include <wrl.h>
#include <d3d12.h>
#include <unordered_map>

class GraphicsDevice;
class SrvManager;

class TextureManager
{
private:
	static TextureManager* instance;

	TextureManager() = default;
	~TextureManager() = default;
	TextureManager(TextureManager&) = delete;
	TextureManager& operator=(TextureManager&) = delete;

	// テクスチャ1枚分のデータ
	struct TextureData {
		std::string filePath;
		DirectX::TexMetadata metadata;
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		uint32_t srvIndex;
		D3D12_CPU_DESCRIPTOR_HANDLE srvHandleCPU;
		D3D12_GPU_DESCRIPTOR_HANDLE srvHandleGPU;
	};

	// テクスチャデータの格納
	std::unordered_map<std::string, TextureData> textureDatas;

	GraphicsDevice* graphicsDevice_ = nullptr;
	SrvManager* srvManager_ = nullptr;

	// SRVインデックスの開始番号
	static uint32_t kSRVIndexTop;

public:
	// シングルトンインスタンスの取得
	static TextureManager* GetInstance();
	// 終了
	void Finalize();
	// 初期化
	void Initialize(GraphicsDevice* graphicsDevice, SrvManager* srvManager);
	// テクスチャ読み込み
	void LoadTexture(const std::string& filePath);
	// ファイルパスからテクスチャ番号を取得
	uint32_t GetTextureIndexByFilePath(const std::string& filePath);
	// テクスチャ番号からGPUハンドルを取得
	D3D12_GPU_DESCRIPTOR_HANDLE GetSrvHandleGPU(uint32_t textureIndex);
	// メタデータ取得
	const DirectX::TexMetadata& GetMetaData(const std::string& filePath);
	// SRV数取得
	uint32_t GetSrvIndex(const std::string& filePath);
	//GPUハンドル取得
	D3D12_GPU_BASED_VALIDATION_FLAGS GetSrvHandleGPU(const std::string& filePath);
};

