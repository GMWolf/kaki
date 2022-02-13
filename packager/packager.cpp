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

bool isChildof(const kaki::Package::Type& type) {
    return std::holds_alternative<kaki::Package::TypeId>(type.typeId)
           && std::get<kaki::Package::TypeId>(type.typeId) == kaki::Package::TypeId::ChildOf;
}

void mergePackages(kaki::Package& out, kaki::Package& in) {

    auto entitiesOffset = out.entities.size();
    auto tableOffset = out.tables.size();

    out.entities.insert(out.entities.end(), in.entities.begin(), in.entities.end());
    out.tables.insert(out.tables.end(), in.tables.begin(), in.tables.end());

    for(size_t i = tableOffset; i < out.tables.size(); i++) {
        out.tables[i].entityFirst += entitiesOffset;
        for(auto& type : out.tables[i].types) {
            if(uint64_t* id = std::get_if<uint64_t>(&type.typeId)) {
                *id += entitiesOffset;
            }
            if (type.object) {
                if (uint64_t *id = std::get_if<uint64_t>(&*type.object)) {
                    *id += entitiesOffset;
                }
            }
        }
        auto& table = out.tables[i];
        if (std::find_if(table.types.begin(), table.types.end(), isChildof) == table.types.end()) {
            table.types.insert(table.types.begin(), kaki::Package::Type{kaki::Package::TypeId::ChildOf, 0u});
            table.typeData.insert(table.typeData.begin(), kaki::Package::Data{});
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
        if (a.types.size() != b.types.size()) {
            return a.types.size() < b.types.size();
        }

        for(int i = 0; i < a.types.size(); i++) {
            if (!typesEqual(a.types[i], b.types[i])) {
                return compareType(a.types[i], b.types[i]);
            }
        }
        return false;
    }
};

bool tableTypesEqual(const kaki::Package::Table& a, const kaki::Package::Table& b) {
    if (a.types.size() != b.types.size()) {
        return false;
    }

    for(int i = 0; i < a.types.size(); i++) {
        if (!typesEqual(a.types[i], b.types[i])) {
            return false;
        }
    }
    return true;
}

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

int main(int argc, char* argv[]) {

    auto out = argv[1];

    kaki::Package outPackage{
        .entities = { {"package"}},
        .tables = {kaki::Package::Table{
            .entityFirst = 0,
            .entityCount = 1,
            .types = {},
            .typeData = {},
        }
        }
    };

    for(int i = 2; i < argc; i++) {
        kaki::Package inPackage;
        {
            auto in = argv[i];

            std::ifstream input(in);
            cereal::JSONInputArchive archive(input);
            archive(inPackage);
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