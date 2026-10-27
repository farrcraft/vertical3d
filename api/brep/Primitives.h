/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "BRep.h"

#include <boost/shared_ptr.hpp>

namespace v3d::brep {

/**
 * Polygon primitives, each one unit across and centred on the origin, so a mesh is placed by
 * its transform, not by where its geometry was built.
 *
 * The cylinder and the cone are open: neither has cap faces.
 **/
boost::shared_ptr<BRep> cube();
boost::shared_ptr<BRep> plane();
boost::shared_ptr<BRep> cylinder();
boost::shared_ptr<BRep> cone();

};  // namespace v3d::brep
