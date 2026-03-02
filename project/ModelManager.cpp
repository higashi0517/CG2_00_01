#include "ModelManager.h"
#include "ModelCommon.h"

ModelManager* ModelManager::instance = nullptr;

ModelManager* ModelManager::GetInstance()
{
	if (instance == nullptr)
	{
		instance = new ModelManager();
	}
	return instance;
}

void ModelManager::Finalize()
{
	if (instance != nullptr)
	{
		delete instance;
		instance = nullptr;
	}
}

void  ModelManager::Initialize(GraphicsDevice* graphicsDevice) 
{
	// モデルマネージャの初期化
	modelCommon = new ModelCommon;
	modelCommon->Initialize(graphicsDevice);
}

void ModelManager::LoadModel(const std::string& filePath)
{
	// 読み込み済みモデルを検索
	if (models.contains(filePath)) {
		// 既に読み込まれている場合は何もしない
		return;
	}

	// モデルの生成とファイル読み込み初期化
	std::unique_ptr<Model> model = std::make_unique<Model>();
	model->Initialize(modelCommon, "Resources", filePath);

	// モデルをmapに登録
	models.insert(std::make_pair(filePath, std::move(model)));
}

Model* ModelManager::FindModel(const std::string& filePath)
{
	// 読み込み済みモデルを検索
	if(models.contains(filePath)){
		// 読み込み済みモデルを返す
		return models.at(filePath).get();
	}
	return nullptr;
}