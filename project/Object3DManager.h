#pragma once
#include "GraphicsDevice.h"
#include "Camera.h"

class Object3DManager
{
private:
	// ルートシグネチャ作成
	void CreateRootSignature();
	// グラフィックスパイプラインステート作成
	void CreateGraphicsPipelineState();

	GraphicsDevice* graphicsDevice_ = nullptr;

	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature_;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState_;

	Camera* defaultCamera_ = nullptr;

public:
	// 初期化
	void Initialize(GraphicsDevice* graphicsDevice);
	GraphicsDevice* GetGraphicsDevice() { return graphicsDevice_; }
	// setter
	void SetCommonRenderState();
	void SetDefaultCamera(Camera* camera) { defaultCamera_ = camera; }

	// getter
	Camera* GetDefaultCamera() { return defaultCamera_; }
};

