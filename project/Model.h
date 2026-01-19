#pragma once
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include "Sprite.h"

class ModelManager;

class Model
{
private:
	struct MaterialData {
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
	};

	// modelManagerのポインタ
	ModelManager* modelManager = nullptr;
	// objファイルのデータ
	ModelData modelData;
	// バッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	// バッファリソース内のデータを指すポインタ
	VertexData* vertexData = nullptr;
	// バッファリソースの使い道を補足するバッファビュー
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	// マテリアルバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	// マテリアルデータを指すポインタ
	Material* materialData = nullptr;

	// .mtlファイルの読み取り
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	// .objファイルの読み取り
	static ModelData LoadObjFile(const std::string& directoryPath, const std::string& filename);

public:
	// 初期化
	void Initialize(ModelManager* modelManager);
	// 描画
	void Draw();
};

