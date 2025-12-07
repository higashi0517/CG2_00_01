#pragma once

class GraphicsDevice;
class SpriteManager
{

public:
	void Initialize(GraphicsDevice* graphicsDevice);

	GraphicsDevice* GetGraphicsDevice() const { return graphicsDevice_; }

private:
	// ルートシグネチャの作成
	void CreateRootSignature();
	// グラフィックパイプラインの作成
	void CreateGraphicsPipelineState();
	// 共通描画設定
	void SetCommonRenderState();

	GraphicsDevice* graphicsDevice_;
};

