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

	/*struct MaterialData {
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	struct ModelData {
		std::vector<Sprite::VertexData> vertices;
		MaterialData material;
	};

	struct VertexData {
		Vector4 position;
		Vector2 texcord;
		Vector3 normal;
	};

	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};*/

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	struct DirectionalLight {
		Vector4 color;
		Vector3 direction;
		float intensity;
	};

	////objファイルのデータ
	//ModelData modelData;
	//// バッファリソース
	//Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	//// バッファリソース内のデータを指すポインタ
	//VertexData* vertexData = nullptr;
	//// バッファリソースの使い道を補足するバッファビュー
	//D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	//// .mtlファイルの読み取り
	//static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	//// .objファイルの読み取り
	//static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

	//// マテリアルバッファリソース
	//Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	//// マテリアルデータを指すポインタ
	//Material* materialData = nullptr;

	// WVP行列バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> wvpResource;
	// WVP行列データを指すポインタ
	TransformationMatrix* transformationMatrixData = nullptr;

	// 平行光源バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> directionalLightResource;
	// 平行光源データを指すポインタ
	DirectionalLight* directionalLightData = nullptr;

	Transform transform;
	Transform cameraTransform;

	Model* model = nullptr;

public:
	// 初期化
	void Initialize(Object3DManager* object3DManager);
	// 更新
	void Update();
	// 描画
	void Draw();
	// setter
	void SetModel(Model* model_) { this->model = model_; }
};