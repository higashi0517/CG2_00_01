#include "Sprite.h"
#include "SpriteManager.h"
#include <GraphicsDevice.h>

using namespace Microsoft::WRL;

void Sprite::Initialize(SpriteManager* spriteManager)
{

	// 引数で受け取ってメンバ変数に記録する
	this->spriteManager = spriteManager;

	// 頂点リソ－スを作る
	vertexResource = graphicsDevice->CreateBufferResource(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * modelData.vertices.size());
	vertexBufferView.StrideInBytes = sizeof(VertexData);

	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());
}
