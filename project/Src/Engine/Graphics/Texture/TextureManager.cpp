#include "TextureManager.h"
#include "GraphicsDevice.h"
#include "StringUtility.h"
#include "SrvManager.h"

TextureManager* TextureManager::instance = nullptr;
// ImGuiで0番を使用するため、1番から開始する
uint32_t TextureManager::kSRVIndexTop = 1;

//uint32_t TextureManager::GetTextureIndexByFilePath(const std::string& filePath)
//{
//	// 読み込み済みテクスチャを検索
//	auto it = std::find_if(
//		textureDatas.begin(),
//		textureDatas.end(),
//		[&](TextureData& textureData) {
//			return textureData.filePath == filePath;
//		}
//	);
//	if (it != textureDatas.end()) {
//		// 読み込み済みなら要素番号を渡す
//		uint32_t textureIndex = static_cast<uint32_t>(std::distance(textureDatas.begin(), it));
//		return textureIndex;
//	}
//
//	assert(0);
//	return 0;
//}

// メタデータ取得
const DirectX::TexMetadata& TextureManager::GetMetaData(const std::string& filePath)
{
	// マップに存在するか確認
	assert(textureDatas.contains(filePath));
	return textureDatas[filePath].metadata;
}

// SRVハンドル取得
D3D12_GPU_DESCRIPTOR_HANDLE TextureManager::GetSrvHandleGPU(const std::string& filePath)
{
	// マップに存在するか確認
	assert(textureDatas.contains(filePath));
	return textureDatas[filePath].srvHandleGPU;
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

void TextureManager::Initialize(GraphicsDevice* graphicsDevice, SrvManager* srvManager)
{
	// グラフィックスデバイス
	graphicsDevice_ = graphicsDevice;
	// SRVマネージャー
	srvManager_ = srvManager;
	// SRVの数と同数
	textureDatas.reserve(srvManager_->kMaxSRVCount);
}

void TextureManager::LoadTexture(const std::string& filePath)
{
	// UIとゲームで共有する画像は、SRVやGPUリソースを再作成しない。
	if (textureDatas.contains(filePath)) { return; }
	TextureData& textureData = textureDatas[filePath];

	// SRV確保
	uint32_t srvIndex = srvManager_->Allocate();
	textureData.srvIndex = srvIndex;
	textureData.filePath = filePath;

	textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(textureData.srvIndex);
	textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(textureData.srvIndex);

	// テクスチャ枚数上限
	assert(srvManager_->Check());

	DirectX::ScratchImage image{};
	std::wstring filePathW = StringUtility::ConvertString(filePath);
	HRESULT hr;
	if (filePathW.ends_with(L".dds")) {
		hr = DirectX::LoadFromDDSFile(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, nullptr, image);
	}
	else {
		hr = DirectX::LoadFromWICFile(filePathW.c_str(), DirectX::WIC_FLAGS_FORCE_SRGB, nullptr, image);
	}
	assert(SUCCEEDED(hr));
	// ミップマップの作成

	DirectX::ScratchImage mipImages{};
	if (DirectX::IsCompressed(image.GetMetadata().format)) {
		mipImages = std::move(image);
	}
	else {
		// 最大4段階。ただし1x1などの小さい画像では作成可能な段階数までにする。
		const auto& metadata = image.GetMetadata();
		size_t mipLevels = 1;
		size_t dimension = metadata.width > metadata.height ? metadata.width : metadata.height;
		while (dimension > 1 && mipLevels < 4) {
			dimension >>= 1;
			++mipLevels;
		}
		if (mipLevels == 1) {
			// 1x1画像には縮小段階がないため、元画像をそのまま使用する。
			mipImages = std::move(image);
		}
		else {
			hr = DirectX::GenerateMipMaps(image.GetImages(), image.GetImageCount(), metadata, DirectX::TEX_FILTER_SRGB, mipLevels, mipImages);
		}
	}
	assert(SUCCEEDED(hr));

	// テクスチャデータを追加
	//textureDatas.resize(textureDatas.size() + 1);
	// 追加したテクスチャデータの参照
	//TextureData& textureData = textureDatas.back();
	textureData.filePath = filePath; // ファイルパス
	textureData.metadata = mipImages.GetMetadata(); // テクスチャメタデータの取得
	textureData.resource = graphicsDevice_->CreateTextureResource(textureData.metadata); // テクスチャリソースの生成
	// テクスチャデータの要素数番号をSRVの番号にする
\
	textureData.srvHandleCPU = srvManager_->GetCPUDescriptorHandle(srvIndex);
	textureData.srvHandleGPU = srvManager_->GetGPUDescriptorHandle(srvIndex);

	// metaDataを基にSRVの設定
	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Format = textureData.metadata.format;
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	if (textureData.metadata.IsCubemap()) {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
		srvDesc.TextureCube.MostDetailedMip = 0;
		srvDesc.TextureCube.MipLevels = UINT_MAX;
		srvDesc.TextureCube.ResourceMinLODClamp = 0.0f;
	}
	else {
		srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
		srvDesc.Texture2D.MipLevels = UINT(textureData.metadata.mipLevels);
	}

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

uint32_t TextureManager::GetSrvIndex(const std::string& filePath)
{
	assert(textureDatas.contains(filePath));
	return textureDatas[filePath].srvIndex;
}
