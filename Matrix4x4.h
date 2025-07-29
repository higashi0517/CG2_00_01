#pragma once

struct Matrix4x4
{
	float m[4][4];
};

struct Vector3
{
	float x, y, z;
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
Vector3 Normalize(const Vector3& v);
Vector3 Cross(const Vector3& v1, const Vector3& v2);
float Dot(const Vector3& v1, const Vector3& v2);
Matrix4x4 MakeLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up);
Vector3 operator*(const Matrix4x4& m, const Vector3& v);
Vector3 operator*(const Vector3& v, const Matrix4x4& m);
Vector3 operator+(const Vector3& v1, const Vector3& v2);
Vector3 operator-(const Vector3& v1, const Vector3& v2);
