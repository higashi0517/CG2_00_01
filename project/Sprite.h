#pragma once
#include <Matrix4x4.h>
#include <wrl.h>

class SpriteManager;

class Sprite
{

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

public:
	void Initialize(SpriteManager* spriteManager);

private:
	SpriteManager* spriteManager = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

};

