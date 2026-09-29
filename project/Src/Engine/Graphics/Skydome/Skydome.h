#pragma once
#include "Matrix4x4.h"
#include <d3d12.h>
#include <wrl.h>
#include <cstdint>

class Camera;
class GraphicsDevice;

// 球体OBJのUVへ2Dの空テクスチャを貼る背景描画。
class Skydome {
public:
	void Initialize(GraphicsDevice* graphicsDevice, Camera* camera);
	void Update();
	void Draw();
private:
	void CreatePipeline();
	Camera* camera_ = nullptr;
	GraphicsDevice* graphicsDevice_ = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource_;
	Microsoft::WRL::ComPtr<ID3D12Resource> matrixResource_;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> pipelineState_;
	D3D12_VERTEX_BUFFER_VIEW vertexView_{};
	D3D12_INDEX_BUFFER_VIEW indexView_{};
	Matrix4x4* viewProjection_ = nullptr;
	uint32_t indexCount_ = 0;
	uint32_t textureIndex_ = 0;
};