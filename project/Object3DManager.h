#pragma once
#include "GraphicsDevice.h"

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

public:
	// 初期化
	void Initialize(GraphicsDevice* graphicsDevice);
	GraphicsDevice* GetGraphicsDevice() { return graphicsDevice_; }
	// 共通の描画ステートをセット
	void SetCommonRenderState();
};

