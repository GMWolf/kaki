//
// Created by felix on 11/02/2022.
//

#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <span>
#include <optional>
#include <variant>
#include <cereal/types/vector.hpp>
#include <cereal/types/string.hpp>
#include <cereal/types/optional.hpp>
#include <cereal/types/variant.hpp>
#include <cereal/external/base64.hpp>

namespace kaki {

    struct Package {
        struct Entity {
            std::string name;
        };
        std::vector<Entity> entities;

        enum class TypeId {
            ChildOf,
        };

        using EntityRef = std::variant<TypeId, uint64_t, std::string>;

        struct Type {
            EntityRef typeId;
            std::optional<EntityRef> object;
        };

        struct Data {
            uint64_t offset;
            uint64_t size;
        };

        struct Table {
            uint64_t entityFirst;
            uint64_t entityCount;
            std::vector<Type> types;
            std::vector<Data> typeData;
        };

        std::vector<Table> tables;
        std::string dataFile;
    };

    template<class Archive>
    void serialize(Archive& archive, Package::Data& data) {
        archive(cereal::make_nvp("offset", data.offset));
        archive(cereal::make_nvp("size", data.size));
    }

    template<class Archive>
    void serialize(Archive& archive, Package::Type& type) {
        archive(cereal::make_nvp("id", type.typeId));
        archive(cereal::make_nvp("object", type.object));
    }

    template<class Archive>
    void serialize(Archive& archive, Package::Table& table) {
        archive(cereal::make_nvp("first", table.entityFirst));
        archive(cereal::make_nvp("size", table.entityCount));
        archive(cereal::make_nvp("types", table.types));
        archive(cereal::make_nvp("data", table.typeData));
    }

    template<class Archive>
    void serialize(Archive& archive, Package::Entity& entity) {
        archive(cereal::make_nvp("name", entity.name));
    }

    template<class Archive>
    void serialize(Archive& archive, Package& package) {
        archive(cereal::make_nvp("datafile", package.dataFile));
        archive(cereal::make_nvp("entities", package.entities));
        archive(cereal::make_nvp("tables", package.tables));
    }
}
