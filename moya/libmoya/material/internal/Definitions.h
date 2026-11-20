/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <MaterialXCore/Document.h>

#include <utility>

namespace v3d::moya::material::internal {

/** The MaterialX document the standard libraries are read into, behind material::Library. **/
class Definitions final {
 public:
    explicit Definitions(MaterialX::DocumentPtr document) : document_(std::move(document)) {
    }

    const MaterialX::DocumentPtr & get() const {
        return document_;
    }

 private:
    MaterialX::DocumentPtr document_;
};

};  // namespace v3d::moya::material::internal
