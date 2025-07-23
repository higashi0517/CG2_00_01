#pragma once
#include "Structs.h"
#include <fstream>
#include <sstream>

class LoadObjFile
{
public:

	ModelData LoadObj(const std::string& directoryPath, const std::string& filename);
	MaterialData LoadMaterialTemplate(const std::string& directoryPath, const std::string& filename);

};

