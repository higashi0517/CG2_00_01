#include "TextureManager.h"
#include "GraphicsDevice.h"
#include "StringUtility.h"

TextureManager* TextureManager::instance = nullptr;
// ImGuiで0番を使用するため、1番から開始する
uint32_t TextureManager::kSRVIndexTop = 1;

uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
{
	// 読み込み済みテクスチャを検索
	auto it = std::find_if(
		textureDatas.begin(),
		textureDatas.end(),
		[&](TextureData& textureData) {
			return textureData.filePath == filePath;
		}
	);
	if (it != textureDatas.end()) {
		// 読み込み済みなら要素番号を渡す
		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
		return textureIndex;
	}

	assert(0);
	return 0;
}

D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(uint32_t textureIndex)
{
	// 範囲外指定違反チェック
	assert(textureIndex < textureDatas.size());

	TextureData& textureData = textureDatas[textureIndex];
	return textureData.srvHandleGPU;
}

TextureManager* TextureManager::GetInstance()
{
	if (instance == nullptr) {
		instance = new TextureManager;
	}
	return instance;
}

void TextureManager::Finalize()
{
	delete instance;
	instance = nullptr;
}

void TextureManager::Initialize(GraphicsDevice* graphicsDevice)
{
	// SRVの数と同数
	textureDatas.reserve(GraphicsDevice::kMaxSRVCount);

	// グラフィックスデバイス
	graphicsDevice_ = graphicsDevice;
}

void TextureManager::LoadTexture(const std::string& filePath) 
{
	// 読み込み済みテクスチャを検索
	auto it = std::find_if(
		textureDatas.begin(),
		textureDatas.end(),
		[&](TextureData& textureData) {
			return textureData.filePath == filePath;
		}
	);
	if (it != textureDatas.end()) {
		// 見つかった場合は何もしない
		return;
	}

	// テクスチャ枚数上限
	assert(textureDatas.size() + kSRVIndexTop < GraphicsDevice::kMaxSRVCount);

	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_NONE, nullptr, image);
	assert(SUCCEEDED(hr));

	// ミニマップの作成
	DirectX::ScratchImage mipImages{};
	hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), image.GetMetadata(), DirectX::TEX_FILTER_DEFAULT, 0, mipImages);
	assert(SUCCEEDED(hr));

	// テクスチャデータを追加
	textureDatas.resize(textureDatas.size() + 1);
	// 追加したテクスチャデータの参照
	TextureData& textureData = textureDatas.back();
	textureData.filePath = filePath; // ファイルパス
	textureData.metadata = mipImages.GetMetadata(); // テクスチャメタデータの取得
	textureData.resource = graphicsDevice_->CreateTextureResource(textureData.metadata); // テクスチャリソースの生成
	// テクスチャデータの要素数番号をSRVの番号にする
	uint32_t srvIndex = static_cast<uint32_t>(textureDatas.size() - 1) + kSRVIndexTop;

	textureData.srvHandleCPU = graphicsDevice_->GetSRVCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = graphicsDevice_->GetSRVGPUDescriptorHandle(srvIndex);

	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);

	// SRVを生成
	graphicsDevice_->GetDevice()->CreateShaderResourceView(
		textureData.resource.Get(),
		&srvDesc,
		textureData.srvHandleCPU
	);

	// テクスチャデータ転送（Copyコマンドを積む）
	Microsoft::WRL::ComPtr<ID3D12Resource> intermediateResource =
		graphicsDevice_->UploadTextureData(mipImages, textureData.resource);

	graphicsDevice_->CloseCommandList();
	graphicsDevice_->ExecuteCommandList();
	graphicsDevice_->WaitForGPU();
	graphicsDevice_->ResetCommandList();

	intermediateResource.Reset();
}