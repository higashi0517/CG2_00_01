#include "JsonLoader.h"

#include <cassert>
#include <fstream>
#include <nlohmann/json.hpp>

namespace
{
    void ScanObject(
        const nlohmann::json& object,
        LevelData& levelData)
    {
        assert(object.contains("type"));
        assert(object["type"].is_string());

        const std::string type =
            object["type"].get<std::string>();

        if (type == "MESH")
        {
            ObjectData objectData{};

            if (object.contains("file_name"))
            {
                objectData.fileName =
                    object["file_name"].get<std::string>();
            }

            assert(object.contains("transform"));

            const nlohmann::json& transform =
                object["transform"];

            objectData.translation = {
                transform["translation"][0].get<float>(),
                transform["translation"][2].get<float>(),
                transform["translation"][1].get<float>()
            };

            objectData.rotation = {
                -transform["rotation"][0].get<float>(),
                -transform["rotation"][2].get<float>(),
                -transform["rotation"][1].get<float>()
            };

            objectData.scaling = {
                transform["scaling"][0].get<float>(),
                transform["scaling"][2].get<float>(),
                transform["scaling"][1].get<float>()
            };

            levelData.objects.push_back(objectData);
        }

        // MESH以外のオブジェクトにもchildrenがある可能性があるため、
        // if (type == "MESH") の外側に書く
        if (object.contains("children"))
        {
            assert(object["children"].is_array());

            for (const nlohmann::json& child :
                object["children"])
            {
                ScanObject(child, levelData);
            }
        }
    }
}

LevelData JsonLoader::Load(const std::string& fileName)
{
    const std::string fullPath =
        "Resources/LevelData/" + fileName + ".json";

    std::ifstream file(fullPath);
    assert(file.is_open());

    nlohmann::json deserialized;
    file >> deserialized;

    assert(deserialized.is_object());
    assert(deserialized.contains("name"));
    assert(deserialized["name"].is_string());
    assert(deserialized["name"] == "scene");

    assert(deserialized.contains("objects"));
    assert(deserialized["objects"].is_array());

    LevelData levelData;

    levelData.objects.reserve(
        deserialized["objects"].size()
    );

    for (const nlohmann::json& object :
        deserialized["objects"])
    {
        ScanObject(object, levelData);
    }

    return levelData;
}