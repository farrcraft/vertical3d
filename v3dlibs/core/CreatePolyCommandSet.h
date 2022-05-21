/**
 * Vertical3D
 * Copyright(c) 2021 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <boost/shared_ptr.hpp>

namespace v3d {
	class BRep;
}
namespace v3d::core {

	boost::shared_ptr<v3d::BRep> create_poly_cube();
	boost::shared_ptr<v3d::BRep> create_poly_plane();
	boost::shared_ptr<v3d::BRep> create_poly_cylinder();
	boost::shared_ptr<v3d::BRep> create_poly_cone();

};
