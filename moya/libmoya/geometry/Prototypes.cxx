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
    return internal::Walk(scene, logger, nullptr).take();
}

Prototypes gather(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger, Cache & cache) {
    return internal::Walk(scene, logger, &cache).take();
}

};  // namespace v3d::moya::geometry
