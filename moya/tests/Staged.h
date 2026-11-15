/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/integrate/Trails.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstdint>
#include <string>
#include <utility>
#include <vector>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace v3d::moya::tracing {

/**
 * A scene of a camera five units back along -z, with no light unless a case adds one. The camera
 * keeps the orthographic projection the scene falls back to, whose screen runs from -1 to 1. A
 * path takes at most depth bounces, and Russian roulette never ends one. Each pixel takes the
 * samples given.
 **/
struct Staged {
    /** The picture's side, in pixels. **/
    static constexpr std::int32_t kSide = 16;

    /** The OpenPBR surface's node. **/
    static constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

    v3d::moya::scene::Scene made { v3d::moya::scene::kinds() };

    /** The scene's options. **/
    v3d::moya::scene::Handle options;

    explicit Staged(std::int32_t depth, std::int32_t samples = 4) {
        options = made.create("options", "options");
        const v3d::moya::scene::Handle camera = made.create("camera", "camera");
        made.set(options, "camera", v3d::moya::scene::Value::handle(camera));
        made.set(options, "resolution", v3d::moya::scene::Value::integers({ kSide, kSide }));
        made.set(options, "samples", v3d::moya::scene::Value::integer(samples));
        made.set(options, "maxDepth", v3d::moya::scene::Value::integer(depth));
        made.set(options, "rouletteDepth", v3d::moya::scene::Value::integer(depth + 1));
        made.set(camera, "transform", v3d::moya::scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    }

    /** A material of an OpenPBR surface with the inputs given. **/
    v3d::moya::scene::Handle material(const std::string & name, const std::vector<std::pair<std::string, v3d::moya::scene::Value>> & inputs) {
        const v3d::moya::scene::Handle surface = made.create("shader", name);
        made.set(surface, "shader", v3d::moya::scene::Value::string(kOpenPbr));
        for (const auto & [input, value] : inputs) {
            made.declare(surface, v3d::moya::scene::Declaration { .name = input, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
            made.set(surface, input, value);
        }
        const v3d::moya::scene::Handle out = made.create("material", name);
        made.set(out, "surface", v3d::moya::scene::Value::handle(surface));
        return out;
    }

    /** A quadrilateral of four corners, in their order, placed with a material, and its mesh. **/
    v3d::moya::scene::Handle quad(const std::string & name, const std::vector<glm::vec3> & corners, const v3d::moya::scene::Handle & material) {
        const v3d::moya::scene::Handle mesh = made.create("mesh", name);
        made.set(mesh, "P", v3d::moya::scene::Value::points(corners));
        made.set(mesh, "vertices", v3d::moya::scene::Value::integers({ 3, 3 }));
        made.set(mesh, "indices", v3d::moya::scene::Value::integers({ 0, 2, 1, 0, 3, 2 }));
        const v3d::moya::scene::Handle placed = instanced(made, name, mesh, {}, glm::mat4(1.0F));
        if (material.valid()) {
            made.set(placed, "material", v3d::moya::scene::Value::handle(material));
        }
        return mesh;
    }

    /** A light of a kind, in a light group, or in none for an empty group. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the kind, the name, then the group
    v3d::moya::scene::Handle light(const std::string & kind, const std::string & name, const std::string & group) {
        const v3d::moya::scene::Handle out = made.create(kind, name);
        made.set(out, "lightGroup", v3d::moya::scene::Value::string(group));
        return out;
    }

    /** The picture, and the events of each pixel's sample zero when trails are given. **/
    v3d::moya::film::Snapshot render(v3d::moya::integrate::Trails * trails) {
        made.commit();
        made.apply();
        v3d::moya::device::CpuDevice device;
        return v3d::moya::integrate::render(device, made, {}, colour::Config(), {}, nullptr, trails);
    }
};

};  // namespace v3d::moya::tracing
