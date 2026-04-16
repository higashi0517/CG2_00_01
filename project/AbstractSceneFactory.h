#pragma once

#include "BaseScene.h"
#include <string>

class AbstractSceneFactory
{
public:
	// 仮想デストラクタ
	virtual ~AbstractSceneFactory() = default;
	// シーンの生成
	virtual BaseScene* CreateScene(const std::string& sceneName) = 0;
};
