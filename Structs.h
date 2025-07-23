#pragma once
#include <vector>
#include <string>
#include <cassert>
#include <wrl.h>
#include <d3d12.h>

// クライアント領域のサイズ
const int32_t kClientWidth = 1280;
const int32_t kClientHeight = 720;
using float32_t = float;

struct Vector2
{
	float32_t x;
	float32_t y;
};

struct Vector3
{
	float32_t x;
	float32_t y;
	float32_t z;
};

struct Vector4
{
	float32_t x;
	float32_t y;
	float32_t z;
	float32_t w;
};
struct Matrix3x3
{
	float32_t m[3][3];
};

struct Matrix4x4
{
	float32_t m[4][4];
};

struct Transform
{
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};

struct VertexData
{
	Vector4 position;
	Vector2 texcoord;
	Vector3 normal;
};

struct Material
{
	Vector4 color;
	int32_t enableLighting;
	float padding[3];
	Matrix4x4 uvTransform;
};

struct TransformationMatrix
{
	Matrix4x4 WVP;
	Matrix4x4 World;
};

struct DirectionalLight
{
	Vector4 color;
	Vector3 direction;
	float intensity;
};

struct MaterialData
{
	std::string textureFilePath;
};

struct ModelData
{
	std::vector<VertexData> vertices;
	MaterialData material;
};


