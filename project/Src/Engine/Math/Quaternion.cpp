#include "Quaternion.h"
#include "Matrix4x4.h"
#include <cmath>

Quaternion Multiply(const Quaternion& lhs, const Quaternion& rhs) {
	Quaternion result{};

	result.w = lhs.w * rhs.w - lhs.x * rhs.x - lhs.y * rhs.y - lhs.z * rhs.z;
	result.x = lhs.w * rhs.x + lhs.x * rhs.w + lhs.y * rhs.z - lhs.z * rhs.y;
	result.y = lhs.w * rhs.y - lhs.x * rhs.z + lhs.y * rhs.w + lhs.z * rhs.x;
	result.z = lhs.w * rhs.z + lhs.x * rhs.y - lhs.y * rhs.x + lhs.z * rhs.w;

	return result;
}

Quaternion IdentityQuaternion() {
	return { 0.0f, 0.0f, 0.0f, 1.0f };
}

Quaternion Conjugate(const Quaternion& quaternion) {
	return {
		-quaternion.x,
		-quaternion.y,
		-quaternion.z,
		 quaternion.w
	};
}

float Norm(const Quaternion& quaternion) {
	return std::sqrt(
		quaternion.x * quaternion.x +
		quaternion.y * quaternion.y +
		quaternion.z * quaternion.z +
		quaternion.w * quaternion.w
	);
}

Quaternion Normalize(const Quaternion& quaternion) {
	float length = Norm(quaternion);

	if (length == 0.0f) {
		return IdentityQuaternion();
	}

	return {
		quaternion.x / length,
		quaternion.y / length,
		quaternion.z / length,
		quaternion.w / length
	};
}

Quaternion Inverse(const Quaternion& quaternion) {
	Quaternion conj = Conjugate(quaternion);
	float normSq =
		quaternion.x * quaternion.x +
		quaternion.y * quaternion.y +
		quaternion.z * quaternion.z +
		quaternion.w * quaternion.w;

	if (normSq == 0.0f) {
		return IdentityQuaternion();
	}

	return {
		conj.x / normSq,
		conj.y / normSq,
		conj.z / normSq,
		conj.w / normSq
	};
}

Quaternion MakeRotateAxisAngleQuaternion(const Vector3& axis, float angle) {

	Vector3 n = Normalize(axis);

	float half = angle * 0.5f;
	float s = std::sin(half);
	float c = std::cos(half);

	return {
		n.x * s,
		n.y * s,
		n.z * s,
		c
	};
}

Quaternion Slerp(const Quaternion& q0_, const Quaternion& q1_, float t) {

	// 念のため正規化（外でやっていても安全側）
	Quaternion q0 = Normalize(q0_);
	Quaternion q1 = Normalize(q1_);

	// 内積（cosθ）
	float dot =
		q0.x * q1.x +
		q0.y * q1.y +
		q0.z * q1.z +
		q0.w * q1.w;

	// 逆側を使ったほうが近い場合（スライドの if(dot < 0) の処理）
	if (dot < 0.0f) {
		q1.x = -q1.x;
		q1.y = -q1.y;
		q1.z = -q1.z;
		q1.w = -q1.w;
		dot = -dot;
	}

	// ほぼ同じ向き → 線形補間で近似（数値安定）
	const float EPSILON = 1e-5f;
	if (dot > 1.0f - EPSILON) {

		Quaternion result{
			q0.x + t * (q1.x - q0.x),
			q0.y + t * (q1.y - q0.y),
			q0.z + t * (q1.z - q0.z),
			q0.w + t * (q1.w - q0.w)
		};

		return Normalize(result);
	}

	// 通常の SLERP
	float theta = std::acos(dot);         // なす角
	float sinTheta = std::sin(theta);

	float scale0 = std::sin((1.0f - t) * theta) / sinTheta;
	float scale1 = std::sin(t * theta) / sinTheta;

	Quaternion result{
		scale0 * q0.x + scale1 * q1.x,
		scale0 * q0.y + scale1 * q1.y,
		scale0 * q0.z + scale1 * q1.z,
		scale0 * q0.w + scale1 * q1.w
	};

	return result; // ※あえてここでは Normalize しない（スライドの注意点どおり）
}