/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../../../api/brep/BRep.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    /**
     * The polygon primitives the Create menu offers, each one unit across and centred on
     * the origin, so a mesh is placed by its transform rather than by where its geometry
     * was built.
     *
     * The cylinder and the cone are open - neither has cap faces, which is what rigel built
     * and what the extrude and split operations were written against.
     **/
    boost::shared_ptr<v3d::brep::BRep> create_poly_cube();
    boost::shared_ptr<v3d::brep::BRep> create_poly_plane();
    boost::shared_ptr<v3d::brep::BRep> create_poly_cylinder();
    boost::shared_ptr<v3d::brep::BRep> create_poly_cone();

};  // namespace v3d::editor
