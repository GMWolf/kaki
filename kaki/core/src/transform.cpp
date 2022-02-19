//
// Created by felix on 16/01/2022.
//

#include "transform.h"
#include <glm/gtx/quaternion.hpp>

kaki::Transform kaki::Transform::inverse() const {
    Transform result{};
    result.orientation = glm::inverse(orientation);
    result.position = result.orientation * ((-position) / scale);
    result.scale = 1.0f / scale;
    return result;
}

glm::mat4 kaki::Transform::matrix() const {
    // TODO: scale
    auto r = glm::toMat3(orientation);
    glm::mat4x3 mat;
    mat[0] = r[0];
    mat[1] = r[1];
    mat[2] = r[2];
    mat[3] = position;
    return mat;
}