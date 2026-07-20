#pragma once
#include <string>
#include <vector>
#include "Matrix4x4.h"

struct ObjectData {

    std::string fileName;
    Vector3 translation;
    Vector3 rotation;
    Vector3 scaling;
};

struct LevelData {
    std::vector<ObjectData> objects;
};

class JsonLoader
{
public:
    static LevelData Load(const std::string& fileName);
};

