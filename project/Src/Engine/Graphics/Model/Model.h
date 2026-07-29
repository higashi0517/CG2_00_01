#pragma once
#include <string>
#include <vector>
#include <d3d12.h>
#include <wrl.h>
#include <unordered_map>
#include "Sprite.h"

struct aiNode;
class ModelCommon;

struct Node {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	std::string name;
	std::vector<Node> children;
};

class Model
{
private:

	struct VertexWeightData {
		float weiight;
		uint32_t vertexIndex;
	};

	struct JointWeightData {
		std::vector<VertexWeightData> vertexWeights;
		Matrix4x4 inverseBindPoseMatrix;
	};

	struct MaterialData {
		std::string textureFilePath;
		uint32_t textureIndex = 0;
	};

	struct VertexData {
		Vector4 position;
		Vector2 texcord;
		Vector3 normal;
	};

	struct ModelData {
		std::unordered_map<std::string, JointWeightData> skinClusterData;
		std::vector<VertexData> vertices;
		std::vector<uint32_t> indices;
		MaterialData material;
		Node rootNode;
	};

	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	// modelManagerのポインタ
	ModelCommon* modelCommon = nullptr;
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

	// インデックスバッファリソース
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;
	// インデックスデータを指すポインタ
	uint32_t* mappedIndex = nullptr;
	// インデックスバッファビュー
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};


public:
	// 初期化
	void Initialize(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename);
	// 描画
	void Draw();
	// .mtlファイルの読み取り
	static MaterialData LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename);
	// .objファイルの読み取り
	static ModelData LoadModelFile(const std::string& directoryPath, const std::string& filename);

	static Node ReadNode(aiNode* node);
	Matrix4x4 GetRootNodeMatrix() const { return modelData.rootNode.localMatrix; }

	std::string GetRootNodeName() const { return modelData.rootNode.name; }

	const Node& GetRootNode() const
	{
		return modelData.rootNode;
	}
};

