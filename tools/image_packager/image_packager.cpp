//
// Created by felix on 13/03/2022.
//

#include <kaki/package.h>
#include <filesystem>
#include <cereal/archives/json.hpp>
#include <fstream>

int main(int argc, char* argv[]) {


    auto inPath = argv[1];
    auto outPath = argv[2];
    auto outBin = std::string(outPath) + ".bin";
    auto name = argv[3];

    kaki::Package package;
    package.dataFile = outBin.substr(outBin.find_last_of("/\\") + 1);

    auto& table = package.tables.emplace_back(kaki::Package::Table{
            .entityFirst = 0,
            .entityCount = 1,
            .components = {
                    kaki::Package::Component{.type = {"kaki::gfx::Image", {}}}
            }
    });

    package.entities.push_back({name});

    std::filesystem::copy(inPath, outBin, std::filesystem::copy_options::overwrite_existing);

    auto& data = table.components[0].data.emplace_back();
    data.offset = 0;
    data.size = std::filesystem::file_size(outBin);

    std::ofstream os(outPath);
    cereal::JSONOutputArchive archive( os );
    archive(package);
}