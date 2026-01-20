#pragma once
#include "Sprite.h"
#include <string>
#include <vector>
#include <d3d12.h>
#include "Model.h"

class Object3DManager;
class Object3D
{
private:
	Object3DManager* object3DManager = nullptr;

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};

	struct CameraForGPU {
		Vector3 worldPosition;
	};

	// WVP行列バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
	// WVP行列データを指すポインタ
	TransformationMatrix* transformationMatrixData = nullptr;

	// 平行光源バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
	// 平行光源データを指すポインタ
	DirectionalLight* directionalLightData = nullptr;
	// カメラ位置バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> cameraResource;
	// カメラ位置データを指すポインタ
	CameraForGPU* cameraData = nullptr;

	Transform transform;
	Transform cameraTransform;

	Model* model = nullptr;

	// 

public:
	// 初期化
	void Initialize(Object3DManager* object3DManager);
	// 更新
	void Update();
	// 描画
	void Draw();
	// setter
	void SetModel(const std::string& filePath);
	//void SetModel(Model* model_) { this->model = model_; }
	void SetTranslate(const Vector3& translate_) { this->transform.translate = translate_; }
	void SetScale(const Vector3& scale_) { this->transform.scale = scale_; }
	void SetRotate(const Vector3& rotate_) { this->transform.rotate = rotate_; }
	// getter
	const Vector3& GetScale() const { return transform.scale; }
	const Vector3& GetRotate() const { return transform.rotate; }
	const Vector3& GetTranslate() const { return transform.translate; }
};