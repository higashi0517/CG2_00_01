#include "Skeleton.h"
#include "Model.h"
#include <algorithm>
#include <cassert>
#include <cstring> 
#include <span>
#include "GraphicsDevice.h"
#include "SrvManager.h"

Skeleton CreateSkeleton(const Node& rootNode)
{
	Skeleton skeleton;
	skeleton.root = CreateJoint(rootNode, {}, skeleton.joints);

	for (const Joint& joint : skeleton.joints) {
		skeleton.jointMap.emplace(joint.name, joint.index);
	}

	UpdateSkeleton(skeleton);

	return skeleton;
}

int32_t CreateJoint(
	const Node& node,
	std::optional<int32_t> parent,
	std::vector<Joint>& joints)
{
	Joint joint;
	joint.name = node.name;
	joint.localMatrix = node.localMatrix;
	joint.skeletonSpaceMatrix = MakeIdentity4x4();
	joint.transform = node.transform;
	joint.index = int32_t(joints.size());
	joint.parent = parent;
	joints.push_back(joint);
	for (const Node& child : node.children) {
		int32_t childIndex = CreateJoint(child, joint.index, joints);
		joints[joint.index].children.push_back(childIndex);
	}

	return joint.index;
}

void UpdateSkeleton(Skeleton& skeleton)
{
	for (Joint& joint : skeleton.joints) {

		joint.localMatrix = MakeAffineMatrix(
			joint.transform.scale,
			joint.transform.rotate,
			joint.transform.translate
		);

		if (joint.parent) {

			joint.skeletonSpaceMatrix = Multiply(joint.localMatrix, skeleton.joints[*joint.parent].skeletonSpaceMatrix);
		}
		else {
			joint.skeletonSpaceMatrix = joint.localMatrix;
		}
	}
}


#ifdef _DEBUG

#include "imgui.h"

namespace
{
	Vector3 GetPosition(const Matrix4x4& matrix)
	{
		return {
			matrix.m[3][0],
			matrix.m[3][1],
			matrix.m[3][2]
		};
	}

	bool WorldToScreen(
		const Vector3& position,
		const Matrix4x4& viewProjectionMatrix,
		float screenWidth,
		float screenHeight,
		ImVec2& screenPosition)
	{
		float clipX =
			position.x * viewProjectionMatrix.m[0][0] +
			position.y * viewProjectionMatrix.m[1][0] +
			position.z * viewProjectionMatrix.m[2][0] +
			viewProjectionMatrix.m[3][0];

		float clipY =
			position.x * viewProjectionMatrix.m[0][1] +
			position.y * viewProjectionMatrix.m[1][1] +
			position.z * viewProjectionMatrix.m[2][1] +
			viewProjectionMatrix.m[3][1];

		float clipZ =
			position.x * viewProjectionMatrix.m[0][2] +
			position.y * viewProjectionMatrix.m[1][2] +
			position.z * viewProjectionMatrix.m[2][2] +
			viewProjectionMatrix.m[3][2];

		float clipW =
			position.x * viewProjectionMatrix.m[0][3] +
			position.y * viewProjectionMatrix.m[1][3] +
			position.z * viewProjectionMatrix.m[2][3] +
			viewProjectionMatrix.m[3][3];

		// カメラより後ろ
		if (clipW <= 0.0f) {
			return false;
		}

		float ndcX = clipX / clipW;
		float ndcY = clipY / clipW;
		float ndcZ = clipZ / clipW;

		// DirectXの画面外
		if (ndcZ < 0.0f || ndcZ > 1.0f) {
			return false;
		}

		screenPosition.x =
			(ndcX + 1.0f) * 0.5f * screenWidth;

		screenPosition.y =
			(1.0f - ndcY) * 0.5f * screenHeight;

		return true;
	}

	Vector3 TransformPoint(
		const Vector3& position,
		const Matrix4x4& matrix)
	{
		return {
			position.x * matrix.m[0][0] +
			position.y * matrix.m[1][0] +
			position.z * matrix.m[2][0] +
			matrix.m[3][0],

			position.x * matrix.m[0][1] +
			position.y * matrix.m[1][1] +
			position.z * matrix.m[2][1] +
			matrix.m[3][1],

			position.x * matrix.m[0][2] +
			position.y * matrix.m[1][2] +
			position.z * matrix.m[2][2] +
			matrix.m[3][2]
		};
	}
}

void DrawSkeletonDebug(
	const Skeleton& skeleton,
	const Matrix4x4& worldMatrix,
	const Matrix4x4& viewProjectionMatrix,
	float screenWidth,
	float screenHeight)
{
	ImDrawList* drawList = ImGui::GetForegroundDrawList();

	for (const Joint& joint : skeleton.joints) {
		if (!joint.parent) {
			continue;
		}

		const Joint& parent =
			skeleton.joints[*joint.parent];

		Vector3 jointPosition =
			GetPosition(joint.skeletonSpaceMatrix);

		Vector3 parentPosition =
			GetPosition(parent.skeletonSpaceMatrix);

		// モデル空間からワールド空間へ変換
		jointPosition =
			TransformPoint(jointPosition, worldMatrix);

		parentPosition =
			TransformPoint(parentPosition, worldMatrix);

		ImVec2 jointScreen;
		ImVec2 parentScreen;

		bool jointVisible = WorldToScreen(
			jointPosition,
			viewProjectionMatrix,
			screenWidth,
			screenHeight,
			jointScreen
		);

		bool parentVisible = WorldToScreen(
			parentPosition,
			viewProjectionMatrix,
			screenWidth,
			screenHeight,
			parentScreen
		);

		if (!jointVisible || !parentVisible) {
			continue;
		}

		// Bone
		drawList->AddLine(
			parentScreen,
			jointScreen,
			IM_COL32(255, 255, 255, 255),
			2.0f
		);

		// Joint
		drawList->AddCircleFilled(
			jointScreen,
			3.0f,
			IM_COL32(0, 255, 0, 255)
		);
	}
}

#endif

SkinCluster CreateSkinCluster(
	GraphicsDevice* graphicsDevice,
	const Skeleton& skeleton,
	const Model::ModelData& modelData,
	const Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>& descriptorHeap,
	uint32_t descriptorSize)
{

	assert(graphicsDevice != nullptr);
	assert(graphicsDevice->GetDevice() != nullptr);

	SkinCluster skinCluster;

	const uint32_t paletteSrvIndex = SrvManager::GetInstance()->Allocate();

	auto device = graphicsDevice->GetDevice();

	// Palette用Resourceを確保
	skinCluster.paletteResource =
		graphicsDevice->CreateBufferResource(
			sizeof(WellForGPU) * skeleton.joints.size()
		);

	WellForGPU* mappedPalette = nullptr;

	skinCluster.paletteResource->Map(
		0,
		nullptr,
		reinterpret_cast<void**>(&mappedPalette)
	);

	skinCluster.mappedPalette = {
		mappedPalette,
		skeleton.joints.size()
	};

	// SRVハンドルを取得
	skinCluster.paletteSrvHandle.first =
		descriptorHeap->GetCPUDescriptorHandleForHeapStart();

	skinCluster.paletteSrvHandle.first.ptr +=
		static_cast<SIZE_T>(descriptorSize) * paletteSrvIndex;

	skinCluster.paletteSrvHandle.second =
		descriptorHeap->GetGPUDescriptorHandleForHeapStart();

	skinCluster.paletteSrvHandle.second.ptr +=
		static_cast<UINT64>(descriptorSize) * paletteSrvIndex;

	// Palette用SRVを作成
	D3D12_SHADER_RESOURCE_VIEW_DESC paletteSrvDesc{};

	paletteSrvDesc.Format =
		DXGI_FORMAT_UNKNOWN;

	paletteSrvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	paletteSrvDesc.ViewDimension =
		D3D12_SRV_DIMENSION_BUFFER;

	paletteSrvDesc.Buffer.FirstElement = 0;

	paletteSrvDesc.Buffer.Flags =
		D3D12_BUFFER_SRV_FLAG_NONE;

	paletteSrvDesc.Buffer.NumElements =
		static_cast<UINT>(skeleton.joints.size());

	paletteSrvDesc.Buffer.StructureByteStride =
		sizeof(WellForGPU);

	device->CreateShaderResourceView(
		skinCluster.paletteResource.Get(),
		&paletteSrvDesc,
		skinCluster.paletteSrvHandle.first
	);

	// influence用のResourceを確保。頂点ごとにInfluence情報を追加できるようにする
	skinCluster.influenceResource = graphicsDevice->CreateBufferResource(
		sizeof(VertexInfluence) * modelData.vertices.size()
	);

	VertexInfluence* mappedInfluence = nullptr;

	skinCluster.influenceResource->Map(
		0,
		nullptr,
		reinterpret_cast<void**>(&mappedInfluence)
	);

	std::memset(
		mappedInfluence,
		0,
		sizeof(VertexInfluence) * modelData.vertices.size()
	); // 0埋め。Weightを0にしておく。

	skinCluster.mappedInfluence = {
		mappedInfluence,
		modelData.vertices.size()
	};

	// Influence用のVBVを作成
	skinCluster.influenceBufferView.BufferLocation =
		skinCluster.influenceResource->GetGPUVirtualAddress();

	skinCluster.influenceBufferView.SizeInBytes =
		UINT(
			sizeof(VertexInfluence) *
			modelData.vertices.size()
		);

	skinCluster.influenceBufferView.StrideInBytes =
		sizeof(VertexInfluence);

	//==================================================
// t1：元頂点用SRV
//==================================================

	const size_t inputVertexBufferSize =
		sizeof(Model::VertexData) *
		modelData.vertices.size();

	skinCluster.inputVertexResource =
		graphicsDevice->CreateBufferResource(
			inputVertexBufferSize
		);

	Model::VertexData* mappedInputVertices = nullptr;

	skinCluster.inputVertexResource->Map(
		0,
		nullptr,
		reinterpret_cast<void**>(
			&mappedInputVertices
			)
	);

	std::memcpy(
		mappedInputVertices,
		modelData.vertices.data(),
		inputVertexBufferSize
	);

	const uint32_t inputVertexSrvIndex =
		SrvManager::GetInstance()->Allocate();

	const D3D12_CPU_DESCRIPTOR_HANDLE
		inputVertexSrvCPU =
		SrvManager::GetInstance()
		->GetCPUDescriptorHandle(
			inputVertexSrvIndex
		);

	skinCluster.inputVertexSrvHandle =
		SrvManager::GetInstance()
		->GetGPUDescriptorHandle(
			inputVertexSrvIndex
		);

	D3D12_SHADER_RESOURCE_VIEW_DESC
		inputVertexSrvDesc{};

	inputVertexSrvDesc.Format =
		DXGI_FORMAT_UNKNOWN;

	inputVertexSrvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	inputVertexSrvDesc.ViewDimension =
		D3D12_SRV_DIMENSION_BUFFER;

	inputVertexSrvDesc.Buffer.FirstElement = 0;

	inputVertexSrvDesc.Buffer.NumElements =
		static_cast<UINT>(
			modelData.vertices.size()
			);

	inputVertexSrvDesc.Buffer.StructureByteStride =
		sizeof(Model::VertexData);

	inputVertexSrvDesc.Buffer.Flags =
		D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(
		skinCluster.inputVertexResource.Get(),
		&inputVertexSrvDesc,
		inputVertexSrvCPU
	);

	//==================================================
	// t2：Influence用SRV
	//==================================================

	const uint32_t influenceSrvIndex =
		SrvManager::GetInstance()->Allocate();

	const D3D12_CPU_DESCRIPTOR_HANDLE
		influenceSrvCPU =
		SrvManager::GetInstance()
		->GetCPUDescriptorHandle(
			influenceSrvIndex
		);

	skinCluster.influenceSrvHandle =
		SrvManager::GetInstance()
		->GetGPUDescriptorHandle(
			influenceSrvIndex
		);

	D3D12_SHADER_RESOURCE_VIEW_DESC
		influenceSrvDesc{};

	influenceSrvDesc.Format =
		DXGI_FORMAT_UNKNOWN;

	influenceSrvDesc.Shader4ComponentMapping =
		D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;

	influenceSrvDesc.ViewDimension =
		D3D12_SRV_DIMENSION_BUFFER;

	influenceSrvDesc.Buffer.FirstElement = 0;

	influenceSrvDesc.Buffer.NumElements =
		static_cast<UINT>(
			modelData.vertices.size()
			);

	influenceSrvDesc.Buffer.StructureByteStride =
		sizeof(VertexInfluence);

	influenceSrvDesc.Buffer.Flags =
		D3D12_BUFFER_SRV_FLAG_NONE;

	device->CreateShaderResourceView(
		skinCluster.influenceResource.Get(),
		&influenceSrvDesc,
		influenceSrvCPU
	);

	//==================================================
	// b0：頂点数用ConstantBuffer
	//==================================================

	constexpr size_t kSkinningInformationSize = 256;

	skinCluster.skinningInformationResource =
		graphicsDevice->CreateBufferResource(
			kSkinningInformationSize
		);

	SkinningInformation*
		mappedSkinningInformation = nullptr;

	skinCluster.skinningInformationResource->Map(
		0,
		nullptr,
		reinterpret_cast<void**>(
			&mappedSkinningInformation
			)
	);

	mappedSkinningInformation->numVertices =
		static_cast<uint32_t>(
			modelData.vertices.size()
			);

	// Compute Shader出力用Resourceを作成
	skinCluster.outputVertexResource =
	
	// Compute Shader出力用Resourceを作成
	skinCluster.outputVertexResource =
		graphicsDevice->CreateUAVBufferResource(
			sizeof(Model::VertexData) *
			modelData.vertices.size()
		);

	// Compute Shaderの出力を描画で使うVBV
	skinCluster.outputVertexBufferView.BufferLocation =
		skinCluster.outputVertexResource
		->GetGPUVirtualAddress();

	skinCluster.outputVertexBufferView.SizeInBytes =
		static_cast<UINT>(
			sizeof(Model::VertexData) *
			modelData.vertices.size()
			);

	skinCluster.outputVertexBufferView.StrideInBytes =
		sizeof(Model::VertexData);

	// 出力頂点用UAVを作成
	uint32_t outputUavIndex =
		SrvManager::GetInstance()->Allocate();

	D3D12_CPU_DESCRIPTOR_HANDLE outputUavCPU =
		SrvManager::GetInstance()
		->GetCPUDescriptorHandle(outputUavIndex);

	D3D12_GPU_DESCRIPTOR_HANDLE outputUavGPU =
		SrvManager::GetInstance()
		->GetGPUDescriptorHandle(outputUavIndex);

	D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
	uavDesc.Format = DXGI_FORMAT_UNKNOWN;
	uavDesc.ViewDimension =
		D3D12_UAV_DIMENSION_BUFFER;
	uavDesc.Buffer.FirstElement = 0;
	uavDesc.Buffer.NumElements =
		static_cast<UINT>(
			modelData.vertices.size()
			);
	uavDesc.Buffer.StructureByteStride =
		sizeof(Model::VertexData);
	uavDesc.Buffer.CounterOffsetInBytes = 0;
	uavDesc.Buffer.Flags =
		D3D12_BUFFER_UAV_FLAG_NONE;

	device->CreateUnorderedAccessView(
		skinCluster.outputVertexResource.Get(),
		nullptr,
		&uavDesc,
		outputUavCPU
	);

	// Dispatch時に使用するGPUハンドルを保存
	skinCluster.outputVertexUavHandle =
		outputUavGPU;

	// InverseBindPoseMatrixを格納する場所を作成して、単位行列で埋める
	skinCluster.inverseBindPoseMatrices.resize(
		skeleton.joints.size()
	);

	std::generate(
		skinCluster.inverseBindPoseMatrices.begin(),
		skinCluster.inverseBindPoseMatrices.end(),
		MakeIdentity4x4
	);

	for (const auto& jointWeight : modelData.skinClusterData) {
		// ModelのSkinClusterの情報を解析
		auto it = skeleton.jointMap.find(jointWeight.first);

		// jointWeight.firstはjoint名なので、
		// skeletonに対象となるjointが含まれているか判断
		if (it == skeleton.jointMap.end()) {
			// そんな名前のJointは存在しない。なので次に回す
			continue;
		}

		// (*it).secondにはjointのindexが入っているので、
		// 該当のindexのinverseBindPoseMatrixを代入
		skinCluster.inverseBindPoseMatrices[(*it).second] =
			jointWeight.second.inverseBindPoseMatrix;

		for (const auto& vertexWeight :
			jointWeight.second.vertexWeights) {

			// 該当のvertexIndexのInfluence情報を参照しておく
			auto& currentInfluence =
				skinCluster.mappedInfluence[
					vertexWeight.vertexIndex
				];

			for (uint32_t index = 0;
				index < kNumMaxInfluence;
				++index) {

				// 空いているところに入れる
				if (currentInfluence.weights[index] == 0.0f) {
					// Weight=0が空いている状態なので、
					// その場所にweightとjointのindexを代入
					currentInfluence.weights[index] =
						vertexWeight.weight;

					currentInfluence.jointIndices[index] =
						(*it).second;

					break;
				}
			}
		}
	}

	return skinCluster;
}

void UpdateSkinCluster(
	SkinCluster& skinCluster,
	const Skeleton& skeleton)
{
	assert(
		skinCluster.inverseBindPoseMatrices.size() ==
		skeleton.joints.size()
	);

	assert(
		skinCluster.mappedPalette.size() ==
		skeleton.joints.size()
	);

	for (size_t jointIndex = 0;
		jointIndex < skeleton.joints.size();
		++jointIndex)
	{
		skinCluster.mappedPalette[jointIndex]
			.skeletonSpaceMatrix =
			Multiply(
				skinCluster.inverseBindPoseMatrices[jointIndex],
				skeleton.joints[jointIndex].skeletonSpaceMatrix
			);

		skinCluster.mappedPalette[jointIndex]
			.skeletonSpaceInverseTransposeMatrix =
			Transpose(
				Inverse(
					skinCluster.mappedPalette[jointIndex]
					.skeletonSpaceMatrix
				)
			);
	}
}