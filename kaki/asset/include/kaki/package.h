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

        struct Table {
            uint64_t entityFirst;
            uint64_t entityCount;
            std::vector<Type> types;
            std::vector<std::vector<uint8_t>> typeData;
        };

        std::string name;
        std::vector<Table> tables;
    };

    template<class Archive>
    void serialize(Archive& archive, Package::Type& type) {
        archive(type.typeId, type.object);
    }

    template<class Archive>
    void serialize(Archive& archive, Package::Table& table) {
        archive(table, table.entityCount, table.types, table.typeData);
    }

    template<class Archive>
    void serialize(Archive& archive, Package& package) {
        archive(package.name, package.tables);
    }

}