/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/World.h>

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/geometry/internal/Walk.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::geometry {

namespace {

template <typename T>
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

};  // namespace

Flat flatten(const scene::Scene & scene, const boost::shared_ptr<log::Logger> & logger) {
    internal::Walk walk(scene, logger);
    std::vector<std::uint32_t> chain;
    walk.group(scene::Handle {}, glm::mat4(1.0F), chain);
    return walk.take();
}

World::World(device::Device & device, const Flat & flat) :
    points_(uploaded(device, flat.points)), faces_(uploaded(device, flat.faces)), placements_(uploaded(device, flat.placements)) {
}

kernel::geometry::Placed World::view() const {
    return kernel::geometry::Placed { .points = device::view<glm::vec3>(*points_), .faces = device::view<std::uint32_t>(*faces_),
        .placements = device::view<kernel::geometry::Placement>(*placements_) };
}

std::uint32_t World::triangles() const {
    return view().faces.size;
}

};  // namespace v3d::moya::geometry
