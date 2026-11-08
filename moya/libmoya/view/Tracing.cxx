/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/view/Tracing.h>

#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/device/cpu/Geometry.h>
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
#include <moya/libmoya/kernel/geometry/Motion.h>
#include <moya/libmoya/kernel/geometry/Shapes.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/kernel/geometry/Trace.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <map>
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

/** Traces a list of rays through one traversal, writing each one's hits at its place in out. **/
void traceList(device::CpuDevice & device, const Tracing & tracing, const std::vector<kernel::geometry::Ray> & rays,
    const std::vector<std::size_t> & places, Traced & out) {
    const auto input = uploaded(device, rays);
    const auto nearest = device.allocate(rays.size() * sizeof(kernel::geometry::Hit));
    const auto any = device.allocate(rays.size() * sizeof(std::uint32_t));
    const auto width = static_cast<std::uint32_t>(std::min<std::size_t>(rays.size(), kRow));
    const auto height = static_cast<std::uint32_t>((rays.size() + width - 1) / width);
    const kernel::Array<kernel::geometry::Ray> rayArray = device::view<kernel::geometry::Ray>(*input);
    const kernel::Array<kernel::geometry::Hit> nearestArray = device::view<kernel::geometry::Hit>(*nearest);
    const kernel::Array<std::uint32_t> anyArray = device::view<std::uint32_t>(*any);
    if (tracing.tracer() == Tracer::Hierarchy) {
        const device::cpu::TraceParams params { .traversal = tracing.hierarchy(), .rays = rayArray, .nearest = nearestArray, .any = anyArray,
            .width = width };
        device.launch(width, height, params, device.module().trace);
    } else {
        const kernel::geometry::TraceRays<kernel::geometry::BruteForce> params { .traversal = tracing.bruteForce(), .rays = rayArray,
            .nearest = nearestArray, .any = anyArray, .width = width };
        device.launch(width, height, params, &kernel::geometry::traceRays<kernel::geometry::BruteForce>);
    }
    std::vector<kernel::geometry::Hit> hits(rays.size());
    device.download(*nearest, hits.data(), rays.size() * sizeof(kernel::geometry::Hit));
    std::vector<std::uint32_t> flags(rays.size());
    device.download(*any, flags.data(), rays.size() * sizeof(std::uint32_t));
    for (std::size_t i = 0; i < rays.size(); ++i) {
        out.nearest[places[i]] = hits[i];
        out.any[places[i]] = flags[i] != 0U;
    }
}

};  // namespace

Tracing::Tracing(device::CpuDevice & device, const geometry::Prototypes & prototypes, Tracer tracer, float time) :
    tracer_(tracer), world_(device, geometry::flatten(prototypes, time)) {
    if (tracer == Tracer::Hierarchy) {
        const accel::Assembly assembly = accel::assemble(device, prototypes);
        std::vector<device::cpu::Geometry> geometries;
        geometries.reserve(prototypes.geometries.size());
        for (const geometry::Geometry & geometry : prototypes.geometries) {
            geometries.push_back(
                device::cpu::Geometry { .form = geometry.form, .points = geometry.points, .widths = geometry.widths, .quadric = geometry.quadric });
        }
        nodes_ = uploaded(device, assembly.nodes);
        motionNodes_ = uploaded(device, assembly.motionNodes);
        references_ = uploaded(device, assembly.references);
        points_ = uploaded(device, prototypes.points);
        times_ = uploaded(device, prototypes.times);
        widths_ = uploaded(device, prototypes.widths);
        quadrics_ = uploaded(device, prototypes.quadrics);
        geometries_ = uploaded(device, geometries);
        keys_ = uploaded(device, assembly.keys);
        instances_ = uploaded(device, assembly.instances);
        worldRoot_ = assembly.world;
        open_ = prototypes.open;
        close_ = prototypes.close;
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
    return device::cpu::Traversal { .nodes = device::view<device::cpu::Node>(*nodes_),
        .motionNodes = device::view<device::cpu::MotionNode>(*motionNodes_),
        .references = device::view<std::uint32_t>(*references_),
        .points = device::view<glm::vec3>(*points_),
        .widths = device::view<float>(*widths_),
        .quadrics = device::view<kernel::geometry::Quadric>(*quadrics_),
        .times = device::view<float>(*times_),
        .geometries = device::view<device::cpu::Geometry>(*geometries_),
        .keys = device::view<kernel::geometry::Key>(*keys_),
        .instances = device::view<device::cpu::Instance>(*instances_),
        .world = worldRoot_.node,
        .worldMoving = worldRoot_.moving ? 1U : 0U,
        .open = open_,
        .close = close_ };
}

Traced trace(device::CpuDevice & device, const geometry::Prototypes & prototypes, const std::vector<kernel::geometry::Ray> & rays,
    Tracer tracer) {
    Traced out;
    if (rays.empty()) {
        return out;
    }
    out.nearest.resize(rays.size());
    out.any.resize(rays.size());
    std::vector<std::size_t> every(rays.size());
    for (std::size_t i = 0; i < rays.size(); ++i) {
        every[i] = i;
    }
    if (tracer == Tracer::Hierarchy) {
        const Tracing tracing(device, prototypes, tracer, prototypes.open);
        traceList(device, tracing, rays, every, out);
        return out;
    }
    // the brute-force traversal places the scene once for each time a ray is traced at
    std::map<float, std::vector<std::size_t>> byTime;
    for (std::size_t i = 0; i < rays.size(); ++i) {
        byTime[geometry::heldTime(prototypes, rays[i].time)].push_back(i);
    }
    for (const auto & [time, places] : byTime) {
        std::vector<kernel::geometry::Ray> list;
        list.reserve(places.size());
        for (const std::size_t place : places) {
            list.push_back(rays[place]);
        }
        const Tracing tracing(device, prototypes, tracer, time);
        traceList(device, tracing, list, places, out);
    }
    return out;
}

};  // namespace v3d::moya::view
