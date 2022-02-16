//
// Created by felix on 12/02/2022.
//

#include <kaki/package.h>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <fstream>
#include <algorithm>
#include <cereal_ext.h>
#include <filesystem>
#include <iostream>

namespace fs = std::filesystem;

bool isChildof(const kaki::Package::Component& component) {
    return std::holds_alternative<kaki::Package::TypeId>(component.type.typeId)
           && std::get<kaki::Package::TypeId>(component.type.typeId) == kaki::Package::TypeId::ChildOf;
}

void mergePackages(kaki::Package& out, kaki::Package& in) {

    auto entitiesOffset = out.entities.size();
    auto tableOffset = out.tables.size();

    out.entities.insert(out.entities.end(), in.entities.begin(), in.entities.end());
    out.tables.insert(out.tables.end(), in.tables.begin(), in.tables.end());

    for(size_t i = tableOffset; i < out.tables.size(); i++) {
        out.tables[i].entityFirst += entitiesOffset;

        for(auto& component : out.tables[i].components) {
            if(uint64_t* id = std::get_if<uint64_t>(&component.type.typeId)) {
                *id += entitiesOffset;
            }
            if (component.type.object) {
                if (uint64_t *id = std::get_if<uint64_t>(&*component.type.object)) {
                    *id += entitiesOffset;
                }
            }
        }

        auto& table = out.tables[i];
        if (std::find_if(table.components.begin(), table.components.end(), isChildof) == table.components.end()) {
            table.components.insert(table.components.begin(), kaki::Package::Component{
                .type = {kaki::Package::TypeId::ChildOf, 0u}
            });
        }
    }
}

static bool typesEqual(const kaki::Package::Type& a, const kaki::Package::Type& b) {
    return a.typeId == b.typeId && a.object == b.object;
}

static bool compareType(const kaki::Package::Type& a, const kaki::Package::Type& b) {
    if (a.typeId != b.typeId) {
        return a.typeId < b.typeId;
    } else {
        return a.object < b.object;
    }
}

struct TableTypeCompare {

    bool operator()(const kaki::Package::Table& a, const kaki::Package::Table& b) {
        if (a.components.size() != b.components.size()) {
            return a.components.size() < b.components.size();
        }

        for(int i = 0; i < a.components.size(); i++) {
            if (!typesEqual(a.components[i].type, b.components[i].type)) {
                return compareType(a.components[i].type, b.components[i].type);
            }
        }
        return false;
    }
};

bool tableTypesEqual(const kaki::Package::Table& a, const kaki::Package::Table& b) {
    if (a.components.size() != b.components.size()) {
        return false;
    }

    for(int i = 0; i < a.components.size(); i++) {
        if (!typesEqual(a.components[i].type, b.components[i].type)) {
            return false;
        }
    }
    return true;
}

/*
void mergeTables(kaki::Package& package) {
    std::sort(package.tables.begin(), package.tables.end(), TableTypeCompare());

    auto f = std::adjacent_find(package.tables.begin(), package.tables.end(), tableTypesEqual);
    while(f != package.tables.end()) {
        auto& first = *f;
        auto& second = *(f+1);

        first.entityCount += second.entityCount;
        for(int i = 0; i < first.typeData.size(); i++) {
            first.typeData[i].vec.insert(first.typeData[i].vec.begin(), second.typeData[i].vec.begin(), second.typeData[i].vec.end());
        }

        std::vector<kaki::Package::Entity> moveEntities(package.entities.begin() + second.entityFirst, package.entities.begin() + second.entityFirst + second.entityCount);
        package.entities.erase(package.entities.begin() + second.entityFirst, package.entities.begin() + second.entityFirst + second.entityCount);

        if (second.entityFirst < first.entityFirst) {
            first.entityFirst -= second.entityCount;
        }

        package.entities.insert(package.entities.begin() + first.entityFirst + first.entityCount, moveEntities.begin(), moveEntities.end());

        package.tables.erase(f+1);

        f = std::adjacent_find(package.tables.begin(), package.tables.end(), tableTypesEqual);
    }
}
 */

int main(int argc, char* argv[]) {

    std::string packageName = argv[1];
    std::string out = packageName + ".json";
    auto outData = packageName + ".bin";

    std::ofstream dataOutput(outData, std::ios::binary | std::ios::out);

    kaki::Package outPackage {
        .entities = { {packageName}},
        .tables = {kaki::Package::Table{
            .entityFirst = 0,
            .entityCount = 1,
            .components = {},
        }
        },
        .dataFile = outData.substr(outData.find_last_of("/\\") + 1),
    };

    for(int i = 2; i < argc; i++) {
        fs::path in = argv[i];

        kaki::Package inPackage;
        {
            std::ifstream input(in);
            cereal::JSONInputArchive archive(input);
            archive(inPackage);
        }

        //patch data
        if (!inPackage.dataFile.empty()) {
            auto dataInputPath = in.parent_path() / inPackage.dataFile;
            std::cout << dataInputPath << std::endl;
            std::ifstream dataInput(dataInputPath, std::ios::binary);

            for (auto &table: inPackage.tables) {
                for (auto & c : table.components) {
                    for(auto& d : c.data) {
                        if (d.size > 0) {
                            d.offset += dataOutput.tellp();
                        }
                    }
                }
            }
            dataOutput << dataInput.rdbuf();
        }
        mergePackages(outPackage, inPackage);
    }

    {
        std::ofstream output(out);
        cereal::JSONOutputArchive archive(output);
        archive(outPackage);
    }

    return 0;
}