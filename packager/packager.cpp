//
// Created by felix on 12/02/2022.
//

#include <kaki/package.h>
#include <cereal/cereal.hpp>
#include <cereal/archives/binary.hpp>
#include <cereal/archives/json.hpp>
#include <fstream>


void mergePackages(kaki::Package& out, kaki::Package& in) {

    auto entitiesOffset = out.entities.size();
    auto tableOffset = out.tables.size();

    out.entities.insert(out.entities.end(), in.entities.begin(), in.entities.end());
    out.tables.insert(out.tables.end(), in.tables.begin(), in.tables.end());

    for(size_t i = tableOffset; i < out.tables.size(); i++) {
        out.tables[i].entityFirst += entitiesOffset;
    }

}

int main(int argc, char* argv[]) {

    auto out = argv[1];

    kaki::Package outPackage{};

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