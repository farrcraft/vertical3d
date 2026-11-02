/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/sl/compiler/Builtins.h>
#include <api/render/offline/sl/runtime/Renderer.h>
#include <api/render/offline/sl/runtime/Value.h>
#include <api/render/offline/texture/Texture.h>

#include <cstddef>
#include <vector>

#include <glm/mat4x4.hpp>

namespace v3d::render::offline::sl::runtime::internal {

/**
 * Everything one call needs, gathered once for the batch. Only the point index changes from
 * one shading point to the next.
 **/
class Site final {
 public:
    compiler::Signature::Body body = compiler::Signature::Body::STUB;
    /** Where the result goes. **/
    Value* target = nullptr;
    /**
     * The value written: the result for every body but a setter. `setxcomp` writes the
     * argument it was given and returns nothing.
     **/
    Value* written = nullptr;
    const std::vector<const Value*>* given = nullptr;
    /** The arguments a body returns results through, for one that writes more than one. **/
    const std::vector<Value*>* outputs = nullptr;
    /** The matrix of a named coordinate space, for the bodies that take one. **/
    glm::mat4x4 matrix = glm::mat4x4(1.0F);
    /** Whether a point through the matrix is divided by w: a space it names is projected. **/
    bool projected = false;
    /** The image texture() reads, and where: its own arguments, or the shader's s and t. **/
    const texture::Texture* texture = nullptr;
    const Value* s = nullptr;
    const Value* t = nullptr;
    /** How the batch lies on the surface, for the derivatives. **/
    Renderer::Shape shape;

    const Value & argument(std::size_t which) const {
        return *(*given)[which];
    }

    std::size_t count() const {
        return given->size();
    }
};

};  // namespace v3d::render::offline::sl::runtime::internal
