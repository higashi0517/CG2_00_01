#include "Sprite.h"
#include "SpriteManager.h"
#include "GraphicsDevice.h"
#include "Matrix4x4.h"
#include <d3d12.h>
#include "WinApp.h"
#include "TextureManager.h"

using namespace Microsoft::WRL;

void Sprite::Initialize(SpriteManager* spriteManager, std::string textureFilePath)
{
	// 引数で受け取ってメンバ変数に記録する
	this->spriteManager = spriteManager;
	// テクスチャ番号を取得
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
	// テクスチャのGPUハンドルを取得
	textureSrvHandleGPU = TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex);

	// 頂点リソ－スを作る
	vertexResource = spriteManager->GetGraphicsDevice()->CreateBufferResource(sizeof(VertexData) * 4);
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * 4);
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	indexResource = spriteManager->GetGraphicsDevice()->CreateBufferResource(sizeof(uint32_t) * 6);

	// インデックスバッファビューの作成
	// リソースの先頭のアドレスから使う
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	// 使用するリソースのサイズはインデックス６つ分のサイズ
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	// インデックスはuint32_t型
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	// インデックスリソースのデータを書き込む
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	// Materialの設定
	materialResource = spriteManager->GetGraphicsDevice()->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// MaterialDataの初期値
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false;
	materialData->uvTransform = MakeIdentity4x4();

	// WVP行列の設定
	wvpResource = spriteManager->GetGraphicsDevice()->CreateBufferResource(sizeof(TransformationMatrix));
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));

	// 単位行列
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
}

void Sprite::Update()
{
	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	// 左右反転
	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	// 上下反転
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureIndex);
	float tex_left = textureLeftTop.x / static_cast<float>(metadata.width);
	float tex_right = (textureLeftTop.x + textureSize.x) / static_cast<float>(metadata.width);
	float tex_top = textureLeftTop.y / static_cast<float>(metadata.height);
	float tex_bottom = (textureLeftTop.y + textureSize.y) / static_cast<float>(metadata.height);

	//// 頂点リソースにデータを書き込む
	//// 左下
	//vertexData[0].position = { 0.0f, 360.0f, 0.0f, 1.0f };
	//vertexData[0].texcoord = { 0.0f, 1.0f };
	//vertexData[0].normal = { 0.0f, 0.0f, 1.0f };

	//// 左上
	//vertexData[1].position = { 0.0f, 0.0f, 0.0f, 1.0f };
	//vertexData[1].texcoord = { 0.0f, 0.0f };
	//vertexData[1].normal = { 0.0f, 0.0f, 1.0f };

	//// 右下
	//vertexData[2].position = { 640.0f, 360.0f, 0.0f, 1.0f };
	//vertexData[2].texcoord = { 1.0f, 1.0f };
	//vertexData[2].normal = { 0.0f, 0.0f, 1.0f };

	//// 右上
	//vertexData[3].position = { 640.0f, 0.0f, 0.0f, 1.0f };
	//vertexData[3].texcoord = { 1.0f, 0.0f };
	//vertexData[3].normal = { 0.0f, 0.0f, 1.0f };

	vertexData[0].position = { left,bottom,0.0f,1.0f };
	vertexData[0].texcoord = { tex_left,tex_bottom };
	vertexData[0].normal = { 0.0f,0.0f,-1.0f };

	vertexData[1].position = { left,top,0.0f,1.0f };
	vertexData[1].texcoord = { tex_left,tex_top };
	vertexData[1].normal = { 0.0f,0.0f,-1.0f };

	vertexData[2].position = { right,bottom,0.0f,1.0f };
	vertexData[2].texcoord = { tex_right,tex_bottom };
	vertexData[2].normal = { 0.0f,0.0f,-1.0f };

	vertexData[3].position = { right,top,0.0f,1.0f };
	vertexData[3].texcoord = { tex_right,tex_top };
	vertexData[3].normal = { 0.0f,0.0f,-1.0f };

	// インデックスリソースにデータを書き込む
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;

	// transform情報を作る
	Transform transform{ {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };

	// positon
	transform.translate = { position.x, position.y, 0.0f };
	// rotation
	transform.rotate = { 0.0f, 0.0f, rotation };
	// size
	transform.scale = { size.x, size.y, 1.0f };

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	transformationMatrixData->WVP = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));;
	transformationMatrixData->World = worldMatrix;
}

void Sprite::Draw()
{
	spriteManager->GetGraphicsDevice()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	spriteManager->GetGraphicsDevice()->GetCommandList()->IASetIndexBuffer(&indexBufferView);

	spriteManager->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	spriteManager->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

	spriteManager->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(textureIndex));

	spriteManager->GetGraphicsDevice()->GetCommandList()->DrawIndexedInstanced(6, 1, 0, 0, 0);

}

void Sprite::ChangeTexture(std::string textureFilePath)
{
	textureIndex = TextureManager::GetInstance()->GetTextureIndexByFilePath(textureFilePath);
}

void Sprite::AdjustTextureSize()
{
	// テクスチャのメタデータを取得
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(textureIndex);

	textureSize.x = static_cast<float>(metadata.width);
	textureSize.y = static_cast<float>(metadata.height);
	// 画像サイズをテクスチャサイズに合わせる
	size = textureSize;
}


