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
class SrvManager;

class Skybox
{
public:

	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};


	GraphicsDevice* graphicsDevice = nullptr;
	Camera* camera = nullptr;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> graphicsPipelineState;
	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};

	struct ConstBufferData {
		Matrix4x4 viewProjection;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> constBuffer;
	ConstBufferData* constMap = nullptr;

	uint32_t textureIndex = 0;

	struct MaterialData {
		Vector4 color;
	};
	Microsoft::WRL::ComPtr<ID3D12Resource> materialResource;
	MaterialData* materialData = nullptr;

	void Initialize(GraphicsDevice* graphicsDevice_, SrvManager* srvManager_, const char* texturePath);	
	void Update(const Matrix4x4& viewMatrix, const Matrix4x4& projectionMatrix);
	void Draw();
	void SetCamera(Camera* camera_) { camera = camera_; }

	void CreateRootSignature();
	void SetCommonRenderState();
	void CreateGraphicsPipelineState();

};

