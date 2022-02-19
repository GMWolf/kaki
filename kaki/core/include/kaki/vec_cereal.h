//
// Created by felix on 19/02/2022.
//

#pragma once

#include <glm/glm.hpp>

namespace cereal {

    template<class Archive, class T, glm::qualifier Q>
    void serialize(Archive &archive, glm::vec<2, T, Q> &v) {
        archive(v.x, v.y);
    }

    template<class Archive, class T, glm::qualifier Q>
    void serialize(Archive &archive, glm::vec<3, T, Q> &v) {
        archive(v.x, v.y, v.z);
    }

    template<class Archive, class T, glm::qualifier Q>
    void serialize(Archive &archive, glm::vec<4, T, Q> &v) {
        archive(v.x, v.y, v.z, v.w);
    }

    template<class Archive, class T, glm::qualifier Q>
    void serialize(Archive &archive, glm::qua<T, Q> &q) {
        archive(q.x, q.y, q.z, q.w);
    }

}