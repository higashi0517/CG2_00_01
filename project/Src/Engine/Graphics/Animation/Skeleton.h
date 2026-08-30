#pragma once
#include "Matrix4x4.h"
#include "Model.h"
#include <string>
#include <vector>
#include <optional>
#include <map>
#include <span>
#include <array>

class GraphicsDevice;

struct Joint {
	QuaternionTransform transform;
	Matrix4x4 localMatrix;
	Matrix4x4 skeletonSpaceMatrix;
	Matrix4x4 worldMatrix;
	std::string name;
	std::vector<int32_t> children;
	int32_t index;
	std::optional<int32_t> parent;
};

struct Skeleton {
	int32_t root;
	std::map<std::string, int32_t> jointMap;
	std::vector<Joint> joints;
};

const uint32_t kNumMaxInfluence = 4;
struct VertexInfluence {
	std::array<float, kNumMaxInfluence> weights;
	std::array<uint32_t, kNumMaxInfluence> jointIndices;
};

struct WellForGPU {
	Matrix4x4 skeletonSpaceMatrix;
	Matrix4x4 skeletonSpaceInverseTransposeMatrix;
};

struct SkinningInformation
{
	uint32_t numVertices;
	uint32_t padding[3];
};

struct SkinCluster {
	std::vector<Matrix4x4> inverseBindPoseMatrices;
	Microsoft::WRL::ComPtr<ID3D12Resource> influenceResource;
	D3D12_VERTEX_BUFFER_VIEW influenceBufferView{};
	std::span<VertexInfluence> mappedInfluence;
	Microsoft::WRL::ComPtr<ID3D12Resource> paletteResource;
	std::span<WellForGPU> mappedPalette;
	std::pair<
		D3D12_CPU_DESCRIPTOR_HANDLE,
		D3D12_GPU_DESCRIPTOR_HANDLE
	> paletteSrvHandle;
	Microsoft::WRL::ComPtr<ID3D12Resource> outputVertexResource;
	D3D12_VERTEX_BUFFER_VIEW outputVertexBufferView{};
	D3D12_GPU_DESCRIPTOR_HANDLE outputVertexUavHandle{};
	// t1
	Microsoft::WRL::ComPtr<ID3D12Resource> inputVertexResource;
	D3D12_GPU_DESCRIPTOR_HANDLE inputVertexSrvHandle{};
	// t2
	D3D12_GPU_DESCRIPTOR_HANDLE influenceSrvHandle{};
	// b0
	Microsoft::WRL::ComPtr<ID3D12Resource> skinningInformationResource;
	SkinningInformation* mappedSkinningInformation = nullptr;
};

Skeleton CreateSkeleton(const Node& rootNode);
int32_t CreateJoint(const Node& node,
	std::optional<int32_t> parent,
	std::vector<Joint>& joints);
void UpdateSkeleton(Skeleton& skeleton);

void DrawSkeletonDebug(
	const Skeleton& skeleton,
	const Matrix4x4& worldMatrix,
	const Matrix4x4& viewProjectionMatrix,
	float screenWidth,
	float screenHeight
);

SkinCluster CreateSkinCluster(
	GraphicsDevice* graphicsDevice,
	const Skeleton& skeleton,
	const Model::ModelData& modelData,
	const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
	uint32_t descriptorSize
);

void UpdateSkinCluster(
	SkinCluster& skinCluster,
	const Skeleton& skeleton
);