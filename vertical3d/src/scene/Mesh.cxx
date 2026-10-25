/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Mesh.h"

#include <api/brep/BRep.h>

#include <cstdint>

#include "Item.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 **/
Mesh::Mesh(const boost::shared_ptr<v3d::brep::BRep>& geometry) :
    geometry_(geometry ? geometry : boost::make_shared<v3d::brep::BRep>()) {
}

/**
 **/
Item::Kind Mesh::kind() const noexcept {
    return KIND;
}

/**
 **/
const boost::shared_ptr<v3d::brep::BRep>& Mesh::geometry() const noexcept {
    return geometry_;
}

/**
 **/
std::uint64_t Mesh::revision() const noexcept {
    return Item::revision() + geometry_->changes();
}

};  // namespace v3d::editor
