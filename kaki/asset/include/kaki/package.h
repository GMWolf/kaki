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
            std::vector<uint8_t> vec;
        };

        struct Table {
            uint64_t entityFirst;
            uint64_t entityCount;
            std::vector<Type> types;
            std::vector<Data> typeData;
        };

        std::vector<Table> tables;
    };

    template<class Archive, cereal::traits::DisableIf<cereal::traits::is_text_archive<Archive>::value> = cereal::traits::sfinae>
    void serialize(Archive& archive, Package::Data& data) {
        archive(data.vec);
    }

    template<class Archive, cereal::traits::EnableIf<cereal::traits::is_text_archive<Archive>::value> = cereal::traits::sfinae>
    std::string save_minimal(const Archive& archive, const Package::Data& data) {
        return cereal::base64::encode(data.vec.data(), data.vec.size());
    }

    template<class Archive, cereal::traits::EnableIf<cereal::traits::is_text_archive<Archive>::value> = cereal::traits::sfinae>
    void load_minimal(const Archive& archive, Package::Data& data, const std::string& value) {
        auto d = cereal::base64::decode(value);
        data.vec = {d.begin(), d.end()};
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
        archive(cereal::make_nvp("entities", package.entities));
        archive(cereal::make_nvp("tables", package.tables));
    }
}
