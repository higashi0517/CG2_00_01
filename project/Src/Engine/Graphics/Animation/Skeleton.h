#pragma once
#include "Matrix4x4.h"
#include <string>
#include <vector>
#include <optional>
#include <map>
#include "Model.h"

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
