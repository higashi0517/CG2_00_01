#include "Object3D.h"
#include "Object3DManager.h"
#include "TextureManager.h"
#include "WinApp.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>

void Object3D::Initialize(Object3DManager* object3DManager)
{
	this->object3DManager = object3DManager;

	// WVP行列バッファの生成
	wvpResource = object3DManager->GetGraphicsDevice()->CreateBufferResource(sizeof(TransformationMatrix));
	// データを書き込むためのポインタを取得
	wvpResource->Map(0, nullptr, reinterpret_cast<void**>(&transformationMatrixData));
	//
	transformationMatrixData->WVP = MakeIdentity4x4();
	transformationMatrixData->World = MakeIdentity4x4();

	// 平行光源バッファの生成
	directionalLightResource = object3DManager->GetGraphicsDevice()->CreateBufferResource(sizeof(DirectionalLight));
	// データを書き込むためのポインタを取得
	directionalLightResource->Map(0, nullptr, reinterpret_cast<void**>(&directionalLightData));
	// 平行光源データの設定
	directionalLightData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	directionalLightData->direction = { 0.0f, -1.0f, 0.0f };
	directionalLightData->intensity = 1.0f;

	// transformの初期化
	transform={
		.scale = {1.0f, 1.0f, 1.0f},
		.rotate = {0.0f, 0.0f, 0.0f},
		.translate = {0.0f, 0.0f, 0.0f}
	};
	cameraTransform = {
		.scale = {1.0f, 1.0f, 1.0f},
		.rotate = {0.3f, 0.0f, 0.0f},
		.translate = {0.0f, 4.0f, -10.0f}
	};
}

void Object3D::Update()
{
	// Transformの更新
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);
	transformationMatrixData->WVP = Multiply(worldMatrix, Multiply(viewMatrix, projectionMatrix));
	transformationMatrixData->World = worldMatrix;

	transform.rotate.y += 0.01f;
}

void Object3D::Draw()
{
	object3DManager->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
	object3DManager->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());
	
	if(model){
		model->Draw();
	}
}