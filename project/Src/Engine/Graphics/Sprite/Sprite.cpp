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
	this->spriteManager = spriteManager;
	this->textureFilePath = textureFilePath;

	textureSrvHandleGPU = TextureManager::GetInstance()->GetSrvHandleGPU((this->textureFilePath));

	vertexResource = spriteManager->GetGraphicsDevice()->CreateBufferResource(sizeof(VertexData) * 4);
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * 4);
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));

	indexResource = spriteManager->GetGraphicsDevice()->CreateBufferResource(sizeof(uint32_t) * 6);

	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = sizeof(uint32_t) * 6;
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&indexData));

	// ★インデックスデータは不変なので初期化時に一度だけ書き込む
	indexData[0] = 0;
	indexData[1] = 1;
	indexData[2] = 2;
	indexData[3] = 1;
	indexData[4] = 3;
	indexData[5] = 2;

	materialResource = spriteManager->GetGraphicsDevice()->CreateBufferResource(sizeof(Material));
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	materialData->color = Vector4(1.0f, 1.0f, 1.0f, 1.0f);
	materialData->enableLighting = false;
	materialData->uvTransform = MakeIdentity4x4();

	wvpResource = spriteManager->GetGraphicsDevice()->CreateBufferResource(sizeof(TransformationMatrix));
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));

	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();
}

void Sprite::Update()
{
	float left = 0.0f - anchorPoint.x;
	float right = 1.0f - anchorPoint.x;
	float top = 0.0f - anchorPoint.y;
	float bottom = 1.0f - anchorPoint.y;

	if (isFlipX_) {
		left = -left;
		right = -right;
	}
	if (isFlipY_) {
		top = -top;
		bottom = -bottom;
	}

	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(this->textureFilePath);
	float tex_left = textureLeftTop.x / static_cast<float>(metadata.width);
	float tex_right = (textureLeftTop.x + textureSize.x) / static_cast<float>(metadata.width);
	float tex_top = textureLeftTop.y / static_cast<float>(metadata.height);
	float tex_bottom = (textureLeftTop.y + textureSize.y) / static_cast<float>(metadata.height);

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

	// ★インデックスデータの書き込みを削除（Initialize に移動済み）

	Transform transform{ {1.0f, 1.0f, 1.0f},{0.0f, 0.0f, 0.0f},{0.0f, 0.0f, 0.0f} };

	transform.translate = { position.x, position.y, 0.0f };
	transform.rotate = { 0.0f, 0.0f, rotation };
	transform.scale = { size.x, size.y, 1.0f };

	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 viewMatrix = MakeIdentity4x4();
	Matrix4x4 projectionMatrix = MakeOrthographicMatrix(0.0f, 0.0f, float(WinApp::kClientWidth), float(WinApp::kClientHeight), 0.0f, 100.0f);
	transformationMatrixData->WVP = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));;
	transformationMatrixData->World = worldMatrix;
}

void Sprite::Draw()
{
	auto* cmdList = spriteManager->GetGraphicsDevice()->GetCommandList().Get();

	cmdList->IASetVertexBuffers(0, 1, &vertexBufferView);
	cmdList->IASetIndexBuffer(&indexBufferView);

	cmdList->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	cmdList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());

	cmdList->SetGraphicsRootDescriptorTable(2, TextureManager::GetInstance()->GetSrvHandleGPU(this->textureFilePath));

	cmdList->DrawIndexedInstanced(6, 1, 0, 0, 0);
}

void Sprite::ChangeTexture(const std::string& textureFilePath)
{
	this->textureFilePath = textureFilePath;
	TextureManager::GetInstance()->LoadTexture(textureFilePath);

}

void Sprite::AdjustTextureSize()
{
	const DirectX::TexMetadata& metadata = TextureManager::GetInstance()->GetMetaData(this->textureFilePath);

	textureSize.x = static_cast<float>(metadata.width);
	textureSize.y = static_cast<float>(metadata.height);
	size = textureSize;
}


