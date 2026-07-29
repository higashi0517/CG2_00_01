#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "Model.h"
#include "TextureManager.h"
#include "ModelCommon.h"
#include <vector>
#include <string>
#include <fstream>
#include <sstream>
#include <cassert>

void Model::Initialize(ModelCommon* modelCommon, const std::string& directorypath, const std::string& filename)
{
	// 引数で受け取ってメンバ変数に記録する
	this->modelCommon = modelCommon;

	// モデル読み込み
	modelData = LoadModelFile(directorypath, filename);

	// 頂点バッファの生成
	vertexResource = modelCommon->GetGraphicsDevice()->CreateBufferResource(sizeof(VertexData) * static_cast<uint32_t>(modelData.vertices.size()));
	vertexBufferView.BufferLocation = vertexResource->GetGPUVirtualAddress();
	vertexBufferView.SizeInBytes = UINT(sizeof(VertexData) * static_cast<uint32_t>(modelData.vertices.size()));
	vertexBufferView.StrideInBytes = sizeof(VertexData);
	// データを書き込むためのポインタを取得
	vertexResource->Map(0, nullptr, reinterpret_cast<void**>(&vertexData));
	//　頂点データを書き込む
	std::memcpy(vertexData, modelData.vertices.data(), sizeof(VertexData) * modelData.vertices.size());

	// インデックスバッファの生成
	indexResource = modelCommon->GetGraphicsDevice()->CreateBufferResource(sizeof(uint32_t) * modelData.indices.size());

	// インデックスバッファビューを設定
	indexBufferView.BufferLocation = indexResource->GetGPUVirtualAddress();
	indexBufferView.SizeInBytes = static_cast<UINT>(sizeof(uint32_t) * modelData.indices.size());
	indexBufferView.Format = DXGI_FORMAT_R32_UINT;

	// インデックスデータを書き込む
	indexResource->Map(0, nullptr, reinterpret_cast<void**>(&mappedIndex));

	std::memcpy(mappedIndex, modelData.indices.data(), sizeof(uint32_t) * modelData.indices.size());

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
	//modelData.material.textureIndex =
		//TextureManager::GetInstance()->GetTextureIndexByFilePath(modelData.material.textureFilePath);
}

void Model::Draw()
{
	// プリミティブトポロジーの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	// 頂点バッファの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->IASetVertexBuffers(0, 1, &vertexBufferView);
	// インデックスバッファの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->IASetIndexBuffer(&indexBufferView);
	// 定数バッファの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootConstantBufferView(0, materialResource->GetGPUVirtualAddress());
	// テクスチャの設定
	modelCommon->GetGraphicsDevice()->GetCommandList()->SetGraphicsRootDescriptorTable(2,
		TextureManager::GetInstance()->GetSrvHandleGPU(modelData.material.textureFilePath));
	// 描画コマンド
	modelCommon->GetGraphicsDevice()->GetCommandList()->DrawIndexedInstanced(static_cast<uint32_t>(modelData.indices.size()), 1, 0, 0, 0);
}

Node Model::ReadNode(aiNode* node)
{
	Node result{};

	aiVector3D scale;
	aiQuaternion rotate;
	aiVector3D translate;

	// Assimpの行列からSRTを取得
	node->mTransformation.Decompose(scale, rotate, translate);

	// 右手座標系から左手座標系に変換
	result.transform.scale = {
		scale.x,
		scale.y,
		scale.z
	};

	result.transform.rotate = {
		rotate.x,
		-rotate.y,
		-rotate.z,
		rotate.w
	};

	result.transform.translate = {
		-translate.x,
		translate.y,
		translate.z
	};

	// 取得したSRTからローカル行列を作り直す
	result.localMatrix = MakeAffineMatrix(
		result.transform.scale,
		result.transform.rotate,
		result.transform.translate
	);

	result.name = node->mName.C_Str();

	result.children.resize(node->mNumChildren);

	for (uint32_t childIndex = 0;
		childIndex < node->mNumChildren;
		++childIndex) {

		result.children[childIndex] =
			ReadNode(node->mChildren[childIndex]);
	}

	return result;
}

// objファイルを読み込む関数
Model::ModelData Model::LoadModelFile(const std::string& directoryPath, const std::string& filename) {

	ModelData modelData;
	Assimp::Importer importer;
	std::string filePath = directoryPath + "/" + filename;
	const aiScene* scene = importer.ReadFile(filePath.c_str(), aiProcess_FlipWindingOrder | aiProcess_FlipUVs);
	modelData.rootNode = ReadNode(scene->mRootNode);
	assert(scene->HasMeshes());

	for (uint32_t meshIndex = 0; meshIndex < scene->mNumMeshes; ++meshIndex) {
		aiMesh* mesh = scene->mMeshes[meshIndex];
		assert(mesh->HasNormals());
		assert(mesh->HasTextureCoords(0));

		const uint32_t vertexOffset = static_cast<uint32_t>(modelData.vertices.size());

		modelData.vertices.resize(vertexOffset + mesh->mNumVertices);

		for (uint32_t vertexIndex = 0; vertexIndex < mesh->mNumVertices; ++vertexIndex) {

			aiVector3D& position = mesh->mVertices[vertexIndex];
			aiVector3D& normal = mesh->mNormals[vertexIndex];
			aiVector3D& texcoord = mesh->mTextureCoords[0][vertexIndex];
			VertexData& vertex = modelData.vertices[vertexOffset + vertexIndex];

			vertex.position = {
				-position.x,
				position.y,
				position.z,
				1.0f
			};

			vertex.normal = {
				-normal.x,
				normal.y,
				normal.z
			};

			vertex.texcord = {
				texcoord.x,
				texcoord.y
			};
		}

		for (uint32_t faceIndex = 0; faceIndex < mesh->mNumFaces; ++faceIndex) {

			aiFace& face = mesh->mFaces[faceIndex];
			assert(face.mNumIndices == 3); // 三角形限定

			for (uint32_t element = 0; element < face.mNumIndices; ++element) {

				uint32_t vertexIndex = vertexOffset + face.mIndices[element];
				modelData.indices.push_back(vertexIndex);
			}
		}


		for (uint32_t materialIndex = 0; materialIndex < scene->mNumMaterials; ++materialIndex) {
			aiMaterial* material = scene->mMaterials[materialIndex];
			if (material->GetTextureCount(aiTextureType_DIFFUSE) != 0) {
				aiString texturePath;
				material->GetTexture(aiTextureType_DIFFUSE, 0, &texturePath);
				modelData.material.textureFilePath = directoryPath + "/" + texturePath.C_Str();
			}
		}

		for (uint32_t boneIndex = 0; boneIndex < mesh->mNumBones; ++boneIndex) {
			aiBone* bone = mesh->mBones[boneIndex];
			std::string jointName = bone->mName.C_Str();
			JointWeightData& jointWeightData = modelData.skinClusterData[jointName];

			aiMatrix4x4 bindPoseMatrixAssimp = bone->mOffsetMatrix.Inverse();
			aiVector3D scale, translate;
			aiQuaternion rotate;
			bindPoseMatrixAssimp.Decompose(scale, rotate, translate);
			Matrix4x4 bindPoseMatrix = MakeAffineMatrix(
				{ scale.x, scale.y, scale.z },
				{ rotate.x, -rotate.y, -rotate.z, rotate.w },
				{ -translate.x, translate.y, translate.z }
			);
			jointWeightData.inverseBindPoseMatrix = Inverse(bindPoseMatrix);

			for (uint32_t weightIndex = 0; weightIndex < bone->mNumWeights; ++weightIndex) {

				jointWeightData.vertexWeights.push_back({
					bone->mWeights[weightIndex].mWeight,
					bone->mWeights[weightIndex].mVertexId
					});
			}
		}
	}

	return modelData;
}