/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/geometry/internal/Displace.h>

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/cpu/Textures.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/geometry/internal/Piece.h>
#include <moya/libmoya/geometry/internal/Programs.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/geometry/subdivision/Surface.h>
#include <moya/libmoya/geometry/subdivision/Tessellation.h>
#include <moya/libmoya/geometry/Surfaces.h>
#include <moya/libmoya/kernel/geometry/Sampled.h>
#include <moya/libmoya/kernel/program/Displacement.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/surface/Frame.h>
#include <moya/libmoya/kernel/surface/Material.h>
#include <moya/libmoya/kernel/surface/Normal.h>
#include <moya/libmoya/kernel/surface/ShadingPoint.h>
#include <moya/libmoya/kernel/surface/Surfaces.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Request.h>
#include <moya/libmoya/texture/Textures.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <sstream>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::moya::geometry::internal {

namespace {

/** A piece's one geometry, as prototypes of their own that hold nothing else. **/
Prototypes alone(const Piece & piece) {
    Prototypes out;
    out.points = piece.points;
    out.widths = piece.widths;
    out.quadrics = piece.quadrics;
    out.controls = piece.controls;
    out.ends = piece.ends;
    out.times = piece.times;
    out.faces = piece.faces;
    out.primvars = piece.primvars;
    out.primvarValues = piece.primvarValues;
    out.primvarSpaces = piece.primvarSpaces;
    out.geometries.push_back(piece.geometry);
    return out;
}

};  // namespace

std::vector<Lift> liftsOf(const Piece & piece, const material::Compiled & displacement, const std::vector<std::uint32_t> & corners,
    const std::vector<float> & normals, const glm::mat4 & placement, const texture::Options & options, const boost::shared_ptr<log::Logger> & logger) {
    const std::size_t places = normals.size() / 3U;
    Prototypes held = alone(piece);
    const kernel::surface::Material running = Programs::append(displacement, held);
    device::CpuDevice device(1);
    const colour::Config config;
    const Surfaces surfaces(device, held, config);
    const kernel::surface::Surfaces view = surfaces.view();
    // a gather given no logger still has its textures log somewhere
    const boost::shared_ptr<log::Logger> logging = logger ? logger : boost::make_shared<log::Logger>();
    const texture::Textures textures(device, held.images, options, config, logging);
    const device::cpu::Textures access = textures.view();

    // the first corner each place lies at
    constexpr std::size_t kNone = std::numeric_limits<std::size_t>::max();
    std::vector<std::size_t> first(places, kNone);
    for (std::size_t corner = 0; corner < corners.size(); ++corner) {
        if (corners[corner] < places && first[corners[corner]] == kNone) {
            first[corners[corner]] = corner;
        }
    }
    const kernel::geometry::Sampled & sampled = piece.geometry.points;
    const float time = sampled.samples > 1U ? piece.times[sampled.times] : 0.0F;
    const glm::mat3 turn { placement };
    const glm::mat3 back = glm::inverse(turn);
    // a normal moves by the inverse transpose of the transformation to the world
    const glm::mat3 normalTurn = glm::transpose(back);
    std::vector<Lift> out(places);
    for (std::size_t place = 0; place < places; ++place) {
        if (first[place] == kNone) {
            continue;
        }
        const auto triangle = static_cast<std::uint32_t>(first[place] / 3U);
        const auto at = static_cast<glm::length_t>(first[place] % 3U);
        const glm::vec3 own { normals[place * 3U], normals[(place * 3U) + 1U], normals[(place * 3U) + 2U] };
        glm::vec3 points[3];
        view.corners(0, triangle, time, points);
        kernel::surface::ShadingPoint point;
        point.geometry = 0;
        point.primitive = triangle;
        point.face = 0;
        point.weights = glm::vec3 { 0.0F };
        point.weights[at] = 1.0F;
        point.uv = glm::vec2 { point.weights.y, point.weights.z };
        point.position = glm::vec3(placement * glm::vec4(points[at], 1.0F));
        point.leave = point.position;
        point.geometric = glm::normalize(normalTurn * own);
        point.shading = point.geometric;
        point.frame = kernel::surface::Frame::around(point.shading);
        point.toWorld = placement;
        point.time = time;
        point.material = kernel::surface::kNoMaterial;
        Lift & lift = out[place];
        lift.along = kernel::program::displacement(view, access, point, running.program);
        const kernel::surface::Frame frame = view.tangentFrame(point);
        const glm::vec3 normal = glm::normalize(own);
        const glm::vec3 along = back * frame.tangent;
        lift.tangent = kernel::surface::unitOrNone(along - (normal * glm::dot(normal, along)));
        if (!(glm::dot(lift.tangent, lift.tangent) > 0.0F)) {
            lift.tangent = kernel::surface::Frame::around(normal).tangent;
        }
        lift.handed = glm::dot(glm::cross(frame.normal, frame.tangent), frame.bitangent) < 0.0F ? -1.0F : 1.0F;
    }
    textures.report();
    return out;
}

bool heldWithin(std::vector<Lift> & lifts, float bound) {
    bool beyond = false;
    for (Lift & lift : lifts) {
        const float length = glm::length(lift.along);
        if (length > bound) {
            beyond = true;
            lift.along *= length > 0.0F ? bound / length : 0.0F;
        }
    }
    return beyond;
}

std::vector<glm::vec3> raised(const subdivision::Surface & surface, const subdivision::Tessellation & tessellation, const std::vector<float> & points,
    const std::vector<Lift> & lifts) {
    std::vector<float> limit = surface.at(points, 3, scene::Interpolation::Vertex, tessellation);
    const std::vector<float> own = surface.normals(points, tessellation);
    for (std::size_t place = 0; place < lifts.size() && (place * 3U) + 2U < limit.size(); ++place) {
        const Lift & lift = lifts[place];
        const glm::vec3 normal = glm::normalize(glm::vec3 { own[place * 3U], own[(place * 3U) + 1U], own[(place * 3U) + 2U] });
        const glm::vec3 tangent = kernel::surface::unitOrNone(lift.tangent - (normal * glm::dot(normal, lift.tangent)));
        const glm::vec3 along = glm::dot(tangent, tangent) > 0.0F ? tangent : lift.tangent;
        const glm::vec3 bitangent = glm::cross(normal, along) * lift.handed;
        const glm::vec3 moved = (along * lift.along.x) + (bitangent * lift.along.y) + (normal * lift.along.z);
        for (glm::length_t axis = 0; axis < 3; ++axis) {
            limit[(place * 3U) + static_cast<std::size_t>(axis)] += moved[axis];
        }
    }
    // a place on a shared edge takes its shared place's point, as the limit's points do
    for (std::size_t place = 0; place < tessellation.shared.size(); ++place) {
        const std::size_t from = static_cast<std::size_t>(tessellation.shared[place]) * 3U;
        std::copy_n(limit.begin() + static_cast<std::ptrdiff_t>(from), 3, limit.begin() + static_cast<std::ptrdiff_t>(place * 3U));
    }
    std::vector<glm::vec3> out;
    out.reserve(tessellation.corners.size());
    for (const std::uint32_t corner : tessellation.corners) {
        const std::size_t at = static_cast<std::size_t>(corner) * 3U;
        out.emplace_back(limit[at], limit[at + 1U], limit[at + 2U]);
    }
    return out;
}

std::string fingerprintOf(const material::Compiled * displacement) {
    if (displacement == nullptr) {
        return {};
    }
    std::ostringstream out;
    for (const kernel::program::Instruction & instruction : displacement->instructions) {
        out << static_cast<unsigned>(instruction.op) << ' ' << instruction.out << ' ' << instruction.width;
        for (std::size_t i = 0; i < kernel::program::kMostOperands; ++i) {
            out << ' ' << instruction.in[i] << ' ' << instruction.widths[i];
        }
        out << '\n';
    }
    for (const float value : displacement->values) {
        out << value << ' ';
    }
    for (const texture::Request & image : displacement->images) {
        out << '\n' << image.file << ' ' << image.colourSpace << ' ' << image.colour;
    }
    return out.str();
}

};  // namespace v3d::moya::geometry::internal
