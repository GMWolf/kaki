//
// Created by felix on 18/04/22.
//

#pragma once

#include <vector>
#include <cstddef>
#include <cstdint>

namespace kaki::ecs {


    using id_t = uint32_t;

    struct EntityId {
        id_t id {};
        uint32_t generation{};

        bool isNull() {
            return generation == 0;
        }
    };

    struct ComponentType {
        id_t component {};
        id_t relationObject {};

        ComponentType() = default;
        inline explicit ComponentType(id_t id) : component(id) {};
        inline explicit ComponentType(const EntityId& component, const EntityId& object) : component(component.id), relationObject(object.id) {};
        inline explicit ComponentType(const ComponentType& component, const EntityId& object) : component(component.component), relationObject(object.id) {};
        inline explicit ComponentType(const EntityId& e) : component(e.id) {
        }
    };

    inline bool operator==(const ComponentType& a, const ComponentType& b) {
        return a.component == b.component && a.relationObject == b.relationObject;
    }

    inline std::strong_ordering operator<=>(const ComponentType& a, const ComponentType& b) {
        if (a.component == b.component) {
            return a.relationObject <=> b.relationObject;
        }
        return a.component <=> b.component;
    }



    struct Type {
        inline Type(std::initializer_list<ComponentType> l) : components(l){
            std::sort(components.begin(), components.end());
        }

        std::vector<ComponentType> components;
    };

    inline auto operator<=>(const Type& a, const Type& b) {
        return a.components <=> b.components;
    }

    inline auto operator==(const Type& a, const Type& b) {
        return a.components == b.components;
    }

}