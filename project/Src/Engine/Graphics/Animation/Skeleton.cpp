#include "Skeleton.h"

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

			joint.skeletonSpaceMatrix = Multiply(joint.localMatrix,skeleton.joints[*joint.parent].skeletonSpaceMatrix);
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
			IM_COL32(255, 0, 0, 255),
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