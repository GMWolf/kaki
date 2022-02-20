//
// Created by felix on 15/02/2022.
//
#define CEREAL_RAPIDJSON_NAMESPACE cereal_rapidjson

#include <fstream>
#include <rapidjson/document.h>
#include <kaki/package.h>
#include <filesystem>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>

namespace fs = std::filesystem;

int main(int argc, char* argv[]) {

    fs::path inputPath = argv[1];

    auto assetName = inputPath.stem();

    fs::path outputFile = argv[2];
    auto outputDataFile = fs::path(outputFile).replace_extension("bin");

    rapidjson::Document doc;
    std::ifstream inputStream(inputPath);
    std::string str((std::istreambuf_iterator<char>(inputStream)),
                    std::istreambuf_iterator<char>());
    doc.ParseInsitu(str.data());


    std::ofstream outputData(outputDataFile, std::ios::binary);
    {
        cereal::BinaryOutputArchive dataArchive(outputData);
        std::string vertex = doc["vertex"].GetString();
        std::string fragment = doc["fragment"].GetString();
        dataArchive(vertex, fragment);
    }

    kaki::Package package {
            .entities = {{assetName}},
            .tables = {kaki::Package::Table{
                    .entityFirst = 0,
                    .entityCount = 1,
                    .components = { {
                        .type = {"kaki::gfx::Pipeline"},
                        .data = {{0, static_cast<uint64_t>(outputData.tellp())}},
                    }
                    },
            }
            },
            .dataFile = outputDataFile.filename(),
    };

    std::ofstream outputStream(outputFile);
    cereal::JSONOutputArchive archive(outputStream);
    archive(package);
    return 0;
}