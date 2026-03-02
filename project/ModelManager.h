#pragma once
#include <map>
#include <string>
#include <memory>
#include "Model.h"
#include "ModelCommon.h"
#include "GraphicsDevice.h"

class ModelManager
{
private:
	static ModelManager* instance;
	// コンストラクタ
	ModelManager() = default;
	// デストラクタ
	~ModelManager() = default;
	// コピーコンストラクタ
	ModelManager(const ModelManager&) = delete;
	// コピー代入演算子
	ModelManager& operator=(const ModelManager&) = delete;
	// モデルデータ
	std::map<std::string, std::unique_ptr<Model>> models;
	// 
	ModelCommon* modelCommon = nullptr;

public:
	// シングルトンインスタンスの取得
	static ModelManager* GetInstance();
	// 終了
	void Finalize();
	// 初期化
	void Initialize(GraphicsDevice* graphicsDevice);
	// モデル読み込み
	void LoadModel(const std::string& filePath);
	// モデルの検索
	Model* FindModel(const std::string& filePath);
};

