#pragma once
#include "Matrix4x4.h"
#include <wrl.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>

class SpriteManager;

class Sprite
{
public:
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

private:
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	SpriteManager* spriteManager = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource;
	Material* materialData = nullptr;

	Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU{};

public:
	void Initialize(SpriteManager* spriteManager);
	void Update();
	void Draw();

};

