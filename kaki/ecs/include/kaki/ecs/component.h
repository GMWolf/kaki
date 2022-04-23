//
// Created by felix on 18/04/22.
//

#pragma once

#include "type.h"
#include <memory>
#include <cassert>

namespace kaki::ecs {

    struct ComponentInfo {
        using DefaultConstructFn = void(void*, size_t);
        using DestructFn = void(void*, size_t);
        using MoveFn = void(void*, void*, size_t);
        using CopyFn = void(void*, void*, size_t);

        size_t size = 0;
        size_t align = 0;

        DefaultConstructFn* constructFn = nullptr;
        DestructFn* destructFn = nullptr;
        MoveFn* moveUninitFn = nullptr;
        MoveFn* moveAssignFn = nullptr;
        MoveFn* moveAssignAndDestructFn = nullptr;
        CopyFn* copyUninitFn = nullptr;
    };

    template<class T>
    ComponentInfo componentInfo() {
        return ComponentInfo{
            .size = sizeof(T),
            .align = sizeof(T),
            .constructFn = [](void* ptr, size_t count) {
                std::uninitialized_default_construct_n(static_cast<T*>(ptr), count);
            },
            .destructFn = [](void* ptr, size_t count) {
                std::destroy_n(static_cast<T*>(ptr), count);
            },
            .moveUninitFn = [](void* to, void* from, size_t count) {
              std::uninitialized_move_n(static_cast<T*>(from), count, static_cast<T*>(to));
            },
            .moveAssignFn = [](void* to, void* from, size_t count) {
                std::move(static_cast<T*>(from), static_cast<T*>(from) + count, static_cast<T*>(to));
            },
            .moveAssignAndDestructFn = [](void* to, void* from, size_t count) {
                std::move(static_cast<T*>(from), static_cast<T*>(from) + count, static_cast<T*>(to));
                std::destroy_n(static_cast<T*>(from), count);
            },
            .copyUninitFn = [](void* to, void* from, size_t count) {
                std::uninitialized_copy_n(static_cast<T*>(from), count, static_cast<T*>(to));
            }
        };
    }

    template<class T>
    struct ComponentTrait {
        static ComponentType id;
    };

    template<class T>
    ComponentType ComponentTrait<T>::id = ComponentType{0};
}