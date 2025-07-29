#include "Matrix4x4.h"
#include <cmath>

// 単位行列の作成
Matrix4x4 MakeIdentity4x4() {

	Matrix4x4 result;

	for (int i = 0; i < 4; i++) {

		for (int j = 0; j < 4; j++) {

			if (i == j) {

				result.m[i][j] = 1.0f;
			} else {
				result.m[i][j] = 0.0f;
			}
		}
	}
	return result;
}

// 平行移動行列
Matrix4x4 MakeTranslateMatrix(const Vector3& translate) {
	Matrix4x4 result = MakeIdentity4x4();
	result.m[3][0] = translate.x;
	result.m[3][1] = translate.y;

	result.m[3][2] = translate.z;
	return result;
}

// 拡大縮小行列
Matrix4x4 MakeScaleMatrix(const Vector3& scale) {
	Matrix4x4 result = MakeIdentity4x4();
	result.m[0][0] = scale.x;
	result.m[1][1] = scale.y;
	result.m[2][2] = scale.z;
	return result;
}

// X軸回転行列
Matrix4x4 MakeRotateXMatrix(float radian) {
	Matrix4x4 result = MakeIdentity4x4();
	result.m[1][1] = std::cos(radian);
	result.m[1][2] = std::sin(radian);
	result.m[2][1] = -std::sin(radian);
	result.m[2][2] = std::cos(radian);
	return result;
}

// Y軸回転行列
Matrix4x4 MakeRotateYMatrix(float radian) {
	Matrix4x4 result = MakeIdentity4x4();
	result.m[0][0] = std::cos(radian);
	result.m[0][2] = -std::sin(radian);
	result.m[2][0] = std::sin(radian);
	result.m[2][2] = std::cos(radian);
	return result;
}

// Z軸回転行列
Matrix4x4 MakeRotateZMatrix(float radian) {
	Matrix4x4 result = MakeIdentity4x4();
	result.m[0][0] = std::cos(radian);
	result.m[0][1] = std::sin(radian);
	result.m[1][0] = -std::sin(radian);
	result.m[1][1] = std::cos(radian);
	return result;
}

// 行列の積
Matrix4x4 Multiply(const Matrix4x4& m1, const Matrix4x4& m2) {

	Matrix4x4 result;

	for (int i = 0; i < 4; i++) {

		for (int j = 0; j < 4; j++) {

			result.m[i][j] = 0.0f;

			for (int k = 0; k < 4; k++) {

				result.m[i][j] += m1.m[i][k] * m2.m[k][j];
			}
		}
	}
	return result;
}

// 3次元アフィン変換行列
Matrix4x4 MakeAffineMatrix(const Vector3& scale, const Vector3& rotate, const Vector3& translate) {

	Matrix4x4 translateMatrix = MakeTranslateMatrix(translate);
	Matrix4x4 rotateXMatrix = MakeRotateXMatrix(rotate.x);
	Matrix4x4 rotateYMatrix = MakeRotateYMatrix(rotate.y);
	Matrix4x4 rotateZMatrix = MakeRotateZMatrix(rotate.z);
	Matrix4x4 scaleMatrix = MakeScaleMatrix(scale);
	Matrix4x4 rotateXYZMatrix = Multiply(rotateXMatrix, Multiply(rotateYMatrix, rotateZMatrix));

	Matrix4x4 result = Multiply(scaleMatrix, rotateXYZMatrix);
	result = Multiply(result, translateMatrix);
	return result;
}

// 透視投影行列
Matrix4x4 MakePerspectiveFovMatrix(float fovY, float aspecRatio, float nearClip, float farClip) {
	Matrix4x4 result = MakeIdentity4x4();

	float f = 1.0f / std::tan(fovY * 0.5f);

	result.m[0][0] = f / aspecRatio;
	result.m[1][1] = f;
	result.m[2][2] = farClip / (farClip - nearClip);
	result.m[2][3] = 1.0f;
	result.m[3][2] = (-nearClip * farClip) / (farClip - nearClip);
	result.m[3][3] = 0.0f;

	return result;
}

// 逆行列を求める関数
Matrix4x4 Inverse(const Matrix4x4& m) {
	Matrix4x4 result{}; // 最後に返す逆行列
	float det = 0.0f;   // 行列式

	// --- 行列式を計算 ---
	{
		float sign = 1.0f;
		for (int i = 0; i < 4; i++) {
			// 小行列を作る
			float subm[3][3];
			int subi = 0;
			for (int row = 1; row < 4; row++) { // 1行目以外
				int subj = 0;
				for (int col = 0; col < 4; col++) {
					if (col == i) continue;
					subm[subi][subj] = m.m[row][col];
					subj++;
				}
				subi++;
			}
			// 小行列の行列式を求める
			float subdet =
				subm[0][0] * (subm[1][1] * subm[2][2] - subm[1][2] * subm[2][1]) -
				subm[0][1] * (subm[1][0] * subm[2][2] - subm[1][2] * subm[2][0]) +
				subm[0][2] * (subm[1][0] * subm[2][1] - subm[1][1] * subm[2][0]);

			// 交互に符号をつけて合計
			det += sign * m.m[0][i] * subdet;
			sign = -sign;
		}
	}

	// --- 行列式が0なら逆行列は存在しない ---
	if (det == 0.0f) {
		return result; // 全部0の行列を返す
	}

	// --- 余因子行列を作って、転置して、行列式で割る ---
	for (int row = 0; row < 4; row++) {
		for (int col = 0; col < 4; col++) {

			// 小行列を作る
			float subm[3][3];
			int subi = 0;
			for (int i = 0; i < 4; i++) {
				if (i == row) continue; // row行目はスキップ
				int subj = 0;
				for (int j = 0; j < 4; j++) {
					if (j == col) continue; // col列目はスキップ
					subm[subi][subj] = m.m[i][j];
					subj++;
				}
				subi++;
			}

			// 小行列の行列式を求める
			float subdet =
				subm[0][0] * (subm[1][1] * subm[2][2] - subm[1][2] * subm[2][1]) -
				subm[0][1] * (subm[1][0] * subm[2][2] - subm[1][2] * subm[2][0]) +
				subm[0][2] * (subm[1][0] * subm[2][1] - subm[1][1] * subm[2][0]);

			// 符号を決める（チェス盤パターン）
			float sign = ((row + col) % 2 == 0) ? 1.0f : -1.0f;

			// 転置して代入（colとrowを逆にする）
			result.m[col][row] = (sign * subdet) / det;
		}
	}
	return result;
}

// 正射影行列
Matrix4x4 MakeOrthographicMatrix(float left, float top, float right, float bottom, float nearClip, float farClip) {
	Matrix4x4 result = MakeIdentity4x4();

	result.m[0][0] = 2.0f / (right - left);
	result.m[1][1] = 2.0f / (top - bottom);
	result.m[2][2] = 1.0f / (farClip - nearClip);
	result.m[3][0] = (left + right) / (left - right);
	result.m[3][1] = (top + bottom) / (bottom - top);
	result.m[3][2] = nearClip / (nearClip - farClip);

	return result;
}

// Normalize関数の実装
Vector3 Normalize(const Vector3& v) {
	float length = std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
	if (length == 0.0f) return { 0.0f, 0.0f, 0.0f }; // ゼロベクトルの正規化は定義されない
	return { v.x / length, v.y / length, v.z / length };
}

// Cross関数の実装
Vector3 Cross(const Vector3& a, const Vector3& b) {
	return {
		a.y * b.z - a.z * b.y,
		a.z * b.x - a.x * b.z,
		a.x * b.y - a.y * b.x
	};
}

// Dot関数
float Dot(const Vector3& a, const Vector3& b) {
	return a.x * b.x + a.y * b.y + a.z * b.z;
}

// スカラー掛け算演算子の定義
Vector3 operator*(const Vector3& v, float s) {
	return { v.x * s, v.y * s, v.z * s };
}
Vector3 operator*(float s, const Vector3& v) {
	return { v.x * s, v.y * s, v.z * s };
}
// 加算演算子
Vector3 operator+(const Vector3& a, const Vector3& b) {
	return { a.x + b.x, a.y + b.y, a.z + b.z };
}
// 減算演算子
Vector3 operator-(const Vector3& a, const Vector3& b) {
	return { a.x - b.x, a.y - b.y, a.z - b.z };
}

Matrix4x4 MakeLookAtMatrix(const Vector3& eye, const Vector3& target, const Vector3& up)
{
	Vector3 zaxis = Normalize(target - eye);        // forward
	Vector3 xaxis = Normalize(Cross(up, zaxis));    // right
	Vector3 yaxis = Cross(zaxis, xaxis);            // up (修正済)

	Matrix4x4 result{};
	result.m[0][0] = xaxis.x;
	result.m[0][1] = yaxis.x;
	result.m[0][2] = zaxis.x;
	result.m[0][3] = 0.0f;

	result.m[1][0] = xaxis.y;
	result.m[1][1] = yaxis.y;
	result.m[1][2] = zaxis.y;
	result.m[1][3] = 0.0f;

	result.m[2][0] = xaxis.z;
	result.m[2][1] = yaxis.z;
	result.m[2][2] = zaxis.z;
	result.m[2][3] = 0.0f;

	result.m[3][0] = -Dot(xaxis, eye);
	result.m[3][1] = -Dot(yaxis, eye);
	result.m[3][2] = -Dot(zaxis, eye);
	result.m[3][3] = 1.0f;

	return result;
}
