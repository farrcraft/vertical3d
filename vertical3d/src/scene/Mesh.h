/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/BRep.h>

#include "Item.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

/**
 * A mesh placed in the scene.
 *
 * The geometry is a brep::BRep described about the item's own origin, and the item's
 * transform places it. The geometry is shared rather than owned, so two items may show one
 * mesh. Component selection lives on the geometry, so two such items share it too.
 **/
class Mesh final : public Item {
 public:
    static constexpr Kind KIND = Kind::Mesh;

    /**
     * @param geometry what the item shows. An empty pointer is replaced by an empty mesh, so
     *        geometry() is never empty
     **/
    explicit Mesh(const boost::shared_ptr<v3d::brep::BRep>& geometry);

    Kind kind() const noexcept override;

    const boost::shared_ptr<v3d::brep::BRep>& geometry() const noexcept;

 private:
    boost::shared_ptr<v3d::brep::BRep> geometry_;
};

};  // namespace v3d::editor
