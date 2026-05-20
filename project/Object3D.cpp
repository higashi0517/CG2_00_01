#include "Object3D.h"
#include "Object3DManager.h"
#include "TextureManager.h"
#include "WinApp.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>
#include "ModelManager.h"
#include "Camera.h"

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
	/*cameraTransform = {
		.scale = {1.0f, 1.0f, 1.0f},
		.rotate = {0.3f, 0.0f, 0.0f},
		.translate = {0.0f, 4.0f, -10.0f}
	};*/

	cameraResource = object3DManager->GetGraphicsDevice()->CreateBufferResource(sizeof(CameraData));
	cameraResource->Map(0, nullptr, reinterpret_cast<void**>(&cameraData));
	cameraData->worldPosition = { 0.0f, 0.0f, 0.0f };

	this->camera = object3DManager->GetDefaultCamera();
}

void Object3D::Update()
{
	// Transformの更新
	Matrix4x4 worldMatrix = MakeAffineMatrix(transform.scale, transform.rotate, transform.translate);
	Matrix4x4 worldViewProjectionMatrix;
	if (camera) {
		const Matrix4x4& viewProjectionMatrix = camera->GetViewProjectionMatrix();
		worldViewProjectionMatrix = Multiply(worldMatrix, viewProjectionMatrix);
	}
	else {
		worldViewProjectionMatrix = worldMatrix;
	}
	/*Matrix4x4 cameraMatrix = MakeAffineMatrix(cameraTransform.scale, cameraTransform.rotate, cameraTransform.translate);
	Matrix4x4 viewMatrix = Inverse(cameraMatrix);
	Matrix4x4 projectionMatrix = MakePerspectiveFovMatrix(0.45f, float(WinApp::kClientWidth) / float(WinApp::kClientHeight), 0.1f, 100.0f);*/
	transformationMatrixData->WVP = worldViewProjectionMatrix;
	transformationMatrixData->World = worldMatrix;

	//transform.rotate.y = +1.0f;

	if (camera) {
		cameraData->worldPosition = camera->GetTranslate();
	}
}

void Object3D::Draw()
{
	auto commandList = object3DManager->GetGraphicsDevice()->GetCommandList();

	// 各定数バッファやディスクリプタテーブルの割り当て
	commandList->SetGraphicsRootConstantBufferView(1, wvpResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootConstantBufferView(3, directionalLightResource->GetGPUVirtualAddress());

	// ★これでエラーが消えます
	commandList->SetGraphicsRootConstantBufferView(5, cameraResource->GetGPUVirtualAddress());
	commandList->SetGraphicsRootDescriptorTable(4, envMapSrvGpuHandle);

	// モデルの描画
	if (model) {
		model->Draw();
	}
}

void Object3D::SetModel(const std::string& filePath)
{
	// モデルを検索してセット
	model = ModelManager::GetInstance()->FindModel(filePath);
}
