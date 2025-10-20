#include "DebugCamera.h"

void DebugCamera::Initialize() {

}

void DebugCamera::Update(const BYTE key[256]) {

		const float speed = 1.0f;
	// 前移動
	if (key[DIK_V]) {


		// カメラ移動ベクトル
		Vector3 move = { 0, 0, speed };
		// 移動ベクトルを角度分だけ回転させる
		Matrix4x4 rotX = MakeRotateXMatrix(rotation_.x);
		Matrix4x4 rotY = MakeRotateYMatrix(rotation_.y);
		Matrix4x4 rotZ = MakeRotateZMatrix(rotation_.z);
		Matrix4x4 rot = Multiply(rotX, Multiply(rotY, rotZ));
		move = TransformNormal(move, rot);

		// 移動ベクトル分だけ座標に加算する
		translation_.x += move.x;
		translation_.y += move.y;
		translation_.z += move.z;
	}

	// 後ろ移動
	if (key[DIK_B]) {

		const float speed = 1.0f;

		// カメラ移動ベクトル
		Vector3 move = { 0, 0, speed };
		// 移動ベクトルを角度分だけ回転させる
		Matrix4x4 rotX = MakeRotateXMatrix(rotation_.x);
		Matrix4x4 rotY = MakeRotateYMatrix(rotation_.y);
		Matrix4x4 rotZ = MakeRotateZMatrix(rotation_.z);
		Matrix4x4 rot = Multiply(rotX, Multiply(rotY, rotZ));
		move = TransformNormal(move, rot);

		// 移動ベクトル分だけ座標に加算する
		translation_.x += move.x;
		translation_.y += move.y;
		translation_.z -= move.z;
	}

	// 右移動
	if (key[DIK_D]) {

		const float speed = 1.0f;

		// カメラ移動ベクトル
		Vector3 move = { speed, 0, 0 };
		// 移動ベクトルを角度分だけ回転させる
		Matrix4x4 rotX = MakeRotateXMatrix(rotation_.x);
		Matrix4x4 rotY = MakeRotateYMatrix(rotation_.y);
		Matrix4x4 rotZ = MakeRotateZMatrix(rotation_.z);
		Matrix4x4 rot = Multiply(rotX, Multiply(rotY, rotZ));
		move = TransformNormal(move, rot);

		// 移動ベクトル分だけ座標に加算する
		translation_.x += move.x;
		translation_.y += move.y;
		translation_.z += move.z;
	}

	// 左移動
	if (key[DIK_A]) {

		const float speed = 1.0f;

		// カメラ移動ベクトル
		Vector3 move = { speed, 0, 0 };
		// 移動ベクトルを角度分だけ回転させる
		Matrix4x4 rotX = MakeRotateXMatrix(rotation_.x);
		Matrix4x4 rotY = MakeRotateYMatrix(rotation_.y);
		Matrix4x4 rotZ = MakeRotateZMatrix(rotation_.z);
		Matrix4x4 rot = Multiply(rotX, Multiply(rotY, rotZ));
		move = TransformNormal(move, rot);

		// 移動ベクトル分だけ座標に加算する
		translation_.x -= move.x;
		translation_.y += move.y;
		translation_.z += move.z;
	}

	// 上移動
	if (key[DIK_W]) {

		const float speed = 1.0f;

		// カメラ移動ベクトル
		Vector3 move = { 0, speed, 0 };
		// 移動ベクトルを角度分だけ回転させる
		Matrix4x4 rotX = MakeRotateXMatrix(rotation_.x);
		Matrix4x4 rotY = MakeRotateYMatrix(rotation_.y);
		Matrix4x4 rotZ = MakeRotateZMatrix(rotation_.z);
		Matrix4x4 rot = Multiply(rotX, Multiply(rotY, rotZ));
		move = TransformNormal(move, rot);

		// 移動ベクトル分だけ座標に加算する
		translation_.x += move.x;
		translation_.y += move.y;
		translation_.z += move.z;
	}

	// 下移動
	if (key[DIK_S]) {

		const float speed = 1.0f;

		// カメラ移動ベクトル
		Vector3 move = { 0, speed, 0 };
		// 移動ベクトルを角度分だけ回転させる
		Matrix4x4 rotX = MakeRotateXMatrix(rotation_.x);
		Matrix4x4 rotY = MakeRotateYMatrix(rotation_.y);
		Matrix4x4 rotZ = MakeRotateZMatrix(rotation_.z);
		Matrix4x4 rot = Multiply(rotX, Multiply(rotY, rotZ));
		move = TransformNormal(move, rot);

		// 移動ベクトル分だけ座標に加算する
		translation_.x += move.x;
		translation_.y -= move.y;
		translation_.z += move.z;
	}

	// 角度から回転行列を計算
	Matrix4x4 rotX = MakeRotateXMatrix(rotation_.x);
	Matrix4x4 rotY = MakeRotateYMatrix(rotation_.y);
	Matrix4x4 rotZ = MakeRotateZMatrix(rotation_.z);
	Matrix4x4 rot = Multiply(rotX, Multiply(rotY, rotZ));

	// 座標から平行移動行列を計算
	Matrix4x4 translate = MakeTranslateMatrix(translation_);

	// 回転行列と平行移動行列からワールド行列を計算
	Matrix4x4 world = Multiply(rot, translate);

	// ワールド行列の逆行列を計算
	viewMatrix_ = Inverse(world);
}