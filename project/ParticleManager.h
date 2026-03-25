#pragma once
#include <unordered_map>
#include <string>
#include <wrl.h>
#include <d3d12.h>
#include "Matrix4x4.h"
#include <list>
#include <random>


class GraphicsDevice;
class Camera;

class ParticleManager
{
public:
	struct Particle {
		Vector3 position;
		Vector3 velocity;
		Vector4 color;
		float currentTime;
		float lifeTime;
	};

	struct ParticleForGPU {
		Matrix4x4 WVP;
		Matrix4x4 World;
		Vector4 color;
	};

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	struct ParticleGroup {
		// マテリアルデータ (テクスチャファイルパスとテクスチャ用SRVインデックス)
		std::string textureFilePath;
		uint32_t textureSrvIndex;

		// パーティクルのリスト (std::list<Particle>型)
		std::list<Particle> particles;

		// インスタンシングデータ用SRVインデックス
		uint32_t instancingSrvIndex;

		// インスタンシングリソース
		Microsoft::WRL::ComPtr<ID3D12Resource> instancingResource;

		// インスタンス数
		uint32_t instanceCount;

		// インスタンシングデータを書き込むためのポインタ
		ParticleForGPU* mappedData;
	};

	static const uint32_t kNumMaxInstance = 10;

	std::unordered_map<std::string, ParticleGroup> particleGroups;
	GraphicsDevice* graphicsDevice = nullptr;
	Camera* camera = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	Material* materialData = nullptr;

	void Initialize(GraphicsDevice* graphicsDevice_);
	void Update();
	void Draw();
	void CreateParticleGroup(const std::string name, const std::string textureFilePath);
	void Emit(const std::string& name, const Vector3& position, uint32_t count);
	void SetCamera(Camera* camera_) { camera = camera_; }

	void CreateRootSignature();
	void SetCommonRenderState();
	void CreateGraphicsPipelineState();

	Particle MakeNewParticle(std::mt19937& randomEngine, const Vector3& translate);

	std::random_device seedGenerator_; // シード生成用
	std::mt19937 randomEngine_;        // メルセンヌ・ツイスタ（エンジン）
};

