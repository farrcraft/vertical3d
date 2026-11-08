/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/view/Tracing.h>

#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/device/cpu/Module.h>
#include <moya/libmoya/device/cpu/Traversal.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/World.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/BruteForce.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Trace.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::view {

namespace {

/** The width of the image a list of rays is laid over, so a launch reaches each. **/
constexpr std::uint32_t kRow = 256;

template <typename T>
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

};  // namespace

Tracing::Tracing(device::CpuDevice & device, const geometry::Prototypes & prototypes, Tracer tracer) :
    tracer_(tracer), world_(device, geometry::flatten(prototypes)) {
    if (tracer == Tracer::Hierarchy) {
        const accel::Assembly assembly = accel::assemble(device, prototypes);
        nodes_ = uploaded(device, assembly.nodes);
        references_ = uploaded(device, assembly.references);
        points_ = uploaded(device, prototypes.points);
        instances_ = uploaded(device, assembly.instances);
        root_ = assembly.world;
    }
}

Tracer Tracing::tracer() const {
    return tracer_;
}

const geometry::World & Tracing::world() const {
    return world_;
}

kernel::geometry::BruteForce Tracing::bruteForce() const {
    return kernel::geometry::BruteForce { .placed = world_.view() };
}

device::cpu::Traversal Tracing::hierarchy() const {
    if (!nodes_) {
        return {};
    }
    return device::cpu::Traversal { .nodes = device::view<device::cpu::Node>(*nodes_), .references = device::view<std::uint32_t>(*references_),
        .points = device::view<glm::vec3>(*points_), .instances = device::view<device::cpu::Instance>(*instances_), .world = root_ };
}

Traced trace(device::CpuDevice & device, const geometry::Prototypes & prototypes, const std::vector<kernel::geometry::Ray> & rays,
    Tracer tracer) {
    Traced out;
    if (rays.empty()) {
        return out;
    }
    const Tracing tracing(device, prototypes, tracer);
    const auto input = uploaded(device, rays);
    const auto nearest = device.allocate(rays.size() * sizeof(kernel::geometry::Hit));
    const auto any = device.allocate(rays.size() * sizeof(std::uint32_t));
    const auto width = static_cast<std::uint32_t>(std::min<std::size_t>(rays.size(), kRow));
    const auto height = static_cast<std::uint32_t>((rays.size() + width - 1) / width);
    const kernel::Array<kernel::geometry::Ray> rayArray = device::view<kernel::geometry::Ray>(*input);
    const kernel::Array<kernel::geometry::Hit> nearestArray = device::view<kernel::geometry::Hit>(*nearest);
    const kernel::Array<std::uint32_t> anyArray = device::view<std::uint32_t>(*any);
    if (tracer == Tracer::Hierarchy) {
        const device::cpu::TraceParams params { .traversal = tracing.hierarchy(), .rays = rayArray, .nearest = nearestArray, .any = anyArray,
            .width = width };
        device.launch(width, height, params, device.module().trace);
    } else {
        const kernel::geometry::TraceRays<kernel::geometry::BruteForce> params { .traversal = tracing.bruteForce(), .rays = rayArray,
            .nearest = nearestArray, .any = anyArray, .width = width };
        device.launch(width, height, params, &kernel::geometry::traceRays<kernel::geometry::BruteForce>);
    }
    out.nearest.resize(rays.size());
    device.download(*nearest, out.nearest.data(), rays.size() * sizeof(kernel::geometry::Hit));
    std::vector<std::uint32_t> flags(rays.size());
    device.download(*any, flags.data(), rays.size() * sizeof(std::uint32_t));
    out.any.reserve(rays.size());
    for (const std::uint32_t flag : flags) {
        out.any.push_back(flag != 0U);
    }
    return out;
}

};  // namespace v3d::moya::view
