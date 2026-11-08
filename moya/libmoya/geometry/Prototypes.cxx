/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/Prototypes.h>

#include <moya/libmoya/geometry/internal/Walk.h>
#include <moya/libmoya/scene/Scene.h>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::geometry {

Prototypes gather(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger) {
    return internal::Walk(scene, logger).take();
}

};  // namespace v3d::moya::geometry
