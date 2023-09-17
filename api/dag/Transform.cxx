/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "Transform.h"


namespace v3d::dag {

    Transform::Transform() : _translation(0.0, 0.0, 0.0), _scale(1.0, 1.0, 1.0) {
    }

    Transform::~Transform() {
    }

    void Transform::scale(const glm::vec3& s) {
        _scale = s;
    }

    void Transform::rotation(const glm::quat& r) {
        _rotation = r;
    }

    void Transform::translation(const glm::vec3& t) {
        _translation += t;
    }

    glm::vec3 Transform::scale(void) const {
        return _scale;
    }

    glm::quat Transform::rotation(void) const {
        return _rotation;
    }

    glm::vec3 Transform::translation(void) const {
        return _translation;
    }

    glm::mat4 Transform::matrix(void) const {
        glm::mat4 trans;
        trans.identity();
        trans.translate(_translation[0], _translation[1], _translation[2]);
        trans.scale(_scale[0], _scale[1], _scale[2]);
        trans *= _rotation.matrix();
        return trans;
    }

};  // namespace v3d::dag
