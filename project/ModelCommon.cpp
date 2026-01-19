#include "ModelCommon.h"

void ModelCommon::Initialize(GraphicsDevice* graphicsDevice) {
	// 引数で受け取ってメンバ変数に記録する
	graphicsDevice_ = graphicsDevice;
	// RootSignatureとGraphicsPipelineStateの作成はここで行う予定
}

