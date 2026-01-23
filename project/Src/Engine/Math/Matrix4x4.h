#pragma once
#include <assert.h>

using float32_t = float;

struct Matrix4x4
{
	float m[4][4];
};

struct Matrix3x3 {
	float32_t m[3][3];
};

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

struct Transform {
	Vector3 scale;
	Vector3 rotate;
	Vector3 translate;
};


// 関数宣言
Matrix4x4 MakeIdentity4x4();
Matrix4x4 MakeTranslateMatrix(const Vector3& translate);
Matrix4x4 MakeScaleMatrix(const Vector3& scale);
Matrix4x4 MakeRotateXMatrix(float radian);
Matrix4x4 MakeRotateYMatrix(float radian);
Matrix4x4 MakeRotateZMatrix(float radian);
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2);
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate);
Matrix4x4 Inverse(const Matrix4x4& m);
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspecRatio, float nearClip, float farClip);
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip);
Matrix4x4 MakeViewportMatrix(float left, float top, float width, float height, float minDepth, float maxDepth);
Vector3 TransformNormal(const Vector3& vector, const Matrix4x4& matrix);

Vector2 operator+(const Vector2& v1, const Vector2& v2);
Vector2& operator+=(Vector2& v1, const Vector2& v2);

Vector3 operator+(const Vector3& v1, const Vector3& v2);
Vector3& operator+=(Vector3& v1, const Vector3& v2);

void Normalize(Vector3& v);