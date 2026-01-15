#pragma once
#include "Matrix4x4.h"
#include <wrl.h>
#include <cstdint>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>

class SpriteManager;

class Sprite
{
public:
	struct VertexData {
		Vector4 position;
		Vector2 texcoord;
		Vector3 normal;
	};

private:
	struct Material {
		Vector4 color;
		int32_t enableLighting;
		float padding[3];
		Matrix4x4 uvTransform;
	};

	struct TransformationMatrix {
		Matrix4x4 WVP;
		Matrix4x4 World;
	};

	Vector2 position = { 0.0f, 0.0f };

	SpriteManager* spriteManager = nullptr;

	Microsoft::WRL::ComPtr<ID3D12Resource> vertexResource;
	Microsoft::WRL::ComPtr<ID3D12Resource> indexResource;

	VertexData* vertexData = nullptr;
	uint32_t* indexData = nullptr;

	D3D12_VERTEX_BUFFER_VIEW vertexBufferView{};
	D3D12_INDEX_BUFFER_VIEW indexBufferView{};

	Microsoft::WRL::ComPtr <ID3D12Resource> materialResource;
	Material* materialData = nullptr;

	Microsoft::WRL::ComPtr <ID3D12Resource> wvpResource;
	TransformationMatrix* transformationMatrixData = nullptr;

	D3D12_GPU_DESCRIPTOR_HANDLE textureSrvHandleGPU{};

	// 回転
	float rotation = 0.0f;
	// サイズ
	Vector2 size = { 640.0f,360.0f };

	// テクスチャ番号
	uint32_t textureIndex = 0;

	// アンカーポイント
	Vector2 anchorPoint = { 0.0f, 0.0f };

	// 左右フリップ
	bool isFlipX_ = false;
	// 上下フリップ
	bool isFlipY_ = false;

	// テクスチャ左上座標
	Vector2 textureLeftTop = { 0.0f, 0.0f };
	// テクスチャ切り出しサイズ
	Vector2 textureSize = { 100.0f, 100.0f };
	// テクスチャサイズをイメージの合わせる
	void AdjustTextureSize();

public:
	void Initialize(SpriteManager* spriteManager,std::string textureFilePath);
	void Update();
	void Draw();
	// テクスチャ変更
	void ChangeTexture(std::string textureFilePath);
	
	// getter
	const Vector2& GetPosition() const { return position; } 			  // 位置
	float GetRotation() const { return rotation; } 						  // 回転
	const Vector4& GetColor() const { return materialData->color; } 	  // 色
	const Vector2& GetSize() const { return size; } 				      // サイズ
	const Vector2& GetAnchorPoint() const { return anchorPoint; }         // アンカーポイント
	const bool& GetIsFlipX() const { return isFlipX_; }                   // 左右フリップ
	const bool& GetIsFlipY() const { return isFlipY_; }                   // 上下フリップ
	const Vector2& GetTextureLeftTop() const { return textureLeftTop; }   // テクスチャ左上座標
	const Vector2& GetTextureSize() const { return textureSize; }         // テクスチャ切り出しサイズ

	// setter
	void SetPosition(const Vector2& pos) { this->position = pos; }                       // 位置
	void SetRotation(float rot) { this->rotation = rot; }                                // 回転
	void SetColor(const Vector4& color) { this->materialData->color = color; }           // 色
	void SetSize(const Vector2& size) { this->size = size; }                             // サイズ
	void SetAnchorPoint(const Vector2& anchorPoint) { this->anchorPoint = anchorPoint; } // アンカーポイント
	void SetIsFlipX(bool isFlipX) { this->isFlipX_ = isFlipX; }                          // 左右フリップ
	void SetIsFlipY(bool isFlipY) { this->isFlipY_ = isFlipY; }                          // 上下フリップ
	void SetTextureLeftTop(const Vector2& leftTop) { this->textureLeftTop = leftTop; }   // テクスチャ左上座標
	void SetTextureSize(const Vector2& size) { this->textureSize = size; }               // テクスチャ切り出しサイズ
};

