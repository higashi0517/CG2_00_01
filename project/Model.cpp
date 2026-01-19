#include "Model.h"
#include "TextureManager.h"
#include "ModelCommon.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>

void Model::Initialize(ModelCommon* modelCommon)
{
	// 引数で受け取ってメンバ変数に記録する
	this->modelCommon = modelCommon;

	// モデル読み込み
	modelData = LoadObjFile("Resources/plane", "plane.obj");

	// 頂点バッファの生成
	vertexResource = modelCommon->GetGraphicsDevice()->CreateBufferResource(sizeof(VertexData) * static_cast<uint32_t>(modelData.vertices.size()));
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * static_cast<uint32_t>(modelData.vertices.size()));
	vertexBufferView.StrideInBytes = sizeof(VertexData);
	// データを書き込むためのポインタを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//　頂点データを書き込む
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	// マテリアルバッファの生成
	materialResource = modelCommon->GetGraphicsDevice()->CreateBufferResource(sizeof(Material));
	// データを書き込むためのポインタを取得
	materialResource->Map(0, nullptr, reinterpret_cast<void**>(&materialData));
	// マテリアルデータの設定
	materialData->color = { 1.0f, 1.0f, 1.0f, 1.0f };
	materialData->enableLighting = true;
	materialData->uvTransform = MakeIdentity4x4();

	// .objの参照しているテクスチャを読み込む
	TextureManager::GetInstance()->LoadTexture(modelData.material.textureFilePath);
	modelData.material.textureIndex =
		TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

void Model::Draw() 
{
	// プリミティブトポロジーの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点バッファの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	// 定数バッファの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	// テクスチャの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootDescriptorTable(2,
		TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureIndex));
	// 描画コマンド
	modelCommon->GetGraphicsDevice()->GetCommandList()->DrawInstanced(static_cast<uint32_t>(modelData.vertices.size()), 1, 0, 0);
}

Model::MaterialData Model::LoadMaterialTemplateFile(const std::string& directoryPath, const std::string& filename)
{
	// 変数の宣言
	MaterialData materialData;
	std::string line;

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	// ファイル読み込み
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;

		// identifierに応じた処理
		if (identifier == "map_Kd") {
			std::string textureFilename;
			s >> textureFilename;
			// 連続してファイルパスする
			materialData.textureFilePath = directoryPath + "/" + textureFilename;
		}
	}
	return materialData;
}

// objファイルを読み込む関数
Model::ModelData Model::LoadObjFile(const std::string& directoryPath, const std::string& filename) {

	// 変数の宣言
	ModelData modelData;
	std::vector<Vector4> positions;
	std::vector<Vector3> normals;
	std::vector<Vector2> texcoords;
	std::string line;

	// ファイルを開く
	std::ifstream file(directoryPath + "/" + filename);
	assert(file.is_open());

	// ファイル読み込み
	while (std::getline(file, line)) {
		std::string identifier;
		std::istringstream s(line);
		s >> identifier;// 先頭の識別子を読む

		// 頂点位置の読み込み
		if (identifier == "v") {

			Vector4 position;
			s >> position.x >> position.y >> position.z;
			position.w = 1.0f; // 同次座標のためwは1.0
			// 位置のxを反転
			position.x *= -1.0f;
			positions.push_back(position);
		}
		else if (identifier == "vt") {

			Vector2 texcoord;
			s >> texcoord.x >> texcoord.y;
			// テクスチャ座標のyを反転
			texcoord.y = 1.0f - texcoord.y;
			texcoords.push_back(texcoord);
		}
		else if (identifier == "vn") {

			Vector3 normal;
			s >> normal.x >> normal.y >> normal.z;
			// 法線のxを反転
			normal.x *= -1.0f;
			normals.push_back(normal);
		}
		else if (identifier == "f") {

			Sprite::VertexData triangle[3];

			// 面は三角形限定、その他は未対応
			for (int32_t faceVertex = 0; faceVertex < 3; ++faceVertex) {
				std::string vertexDefinition;
				s >> vertexDefinition;

				// 頂点の要素へのIndexは「位置/UV/法線」で格納されているので、分解してIndexを取得
				std::istringstream v(vertexDefinition);
				uint32_t elementIndices[3];
				for (int32_t element = 0; element < 3; ++element) {

					std::string index;
					std::getline(v, index, '/');// 区切りでインデックスを読んでいく
					elementIndices[element] = std::stoi(index);
				}

				// 要素へのindexから、実際の要素の値を取得して、頂点を構築する
				Vector4 position = positions[elementIndices[0] - 1];
				Vector2 texcoord = texcoords[elementIndices[1] - 1];
				Vector3 normal = normals[elementIndices[2] - 1];
				//VertexData vetex = { position, texcoord, normal };
				//modelData.vertices.push_back(vetex);
				triangle[faceVertex] = { position, texcoord, normal };
			}

			// 頂点を逆順で登録することで、周り準を逆にする
			modelData.vertices.push_back(triangle[2]);
			modelData.vertices.push_back(triangle[1]);
			modelData.vertices.push_back(triangle[0]);
		}
		else if (identifier == "mtllib") {

			// materialTemplateLibraryファイルの名前を取得する
			std::string materialFilename;
			s >> materialFilename;
			// 基本的にobjファイルと同一階層にmtlは存在させるので、ディレクトリ名とファイル名を渡す
			modelData.material = LoadMaterialTemplateFile(directoryPath, materialFilename);
		}
	}
	return modelData;
}

