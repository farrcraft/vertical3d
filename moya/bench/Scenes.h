/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Scene.h>

#include <cstdint>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

/** The scenes moya_bench times, and the rays it traces through them. **/
namespace v3d::moya::bench {

/**
 * One of the benchmark's fixed scenes, and the box every ray is drawn from or across. Each lies
 * over x and y from -1 to 1, and between the box's low and high z.
 **/
struct Fixed {
    std::string name;
    boost::shared_ptr<scene::Scene> scene;
    glm::vec3 low { -1.0F };
    glm::vec3 high { 1.0F };
};

/**
 * The benchmark's scenes, each the same on every run:
 * - small triangles: a rippled grid of about a million triangles at a scale of 1.
 * - large triangles: 32 triangles, each reaching across most of the box, crossing one another.
 * - curves: 32768 round cubic b-spline curves of six vertices, standing like grass.
 * - instances: a rippled patch of 20000 triangles, four to a group and the group 256 times, so
 *   one patch is placed 1024 times through two levels.
 * - motion: a grid of about 130000 triangles that rises over the shutter, and 64 patches whose
 *   instances move across it.
 *
 * A scale below 1 makes each scene that many times smaller, for a short pass.
 **/
std::vector<Fixed> scenes(float scale);

/**
 * The lit scene the integrator is timed over: a room whose floor is the small triangles' rippled
 * grid, about a million triangles at a scale of 1, with four walls and no ceiling. A white dome
 * lights it from above, and a rect, a disk and a sphere light hang inside it. Every surface is
 * diffuse. A camera above the room looks down into it, at side by side pixels and samples per
 * pixel. The scene is committed and applied, and is the same on every run.
 **/
boost::shared_ptr<scene::Scene> litRoom(float scale, std::uint32_t side, std::uint32_t samples);

/**
 * The lit room with a material of textures on every surface: OpenPBR's base under a coat, whose
 * base colour is an image and whose specular roughness is the image's red. The floor's st runs
 * over it four times along each axis, and the walls have none. The image is written to the path
 * given first, the same on every run. The scene is committed and applied.
 **/
boost::shared_ptr<scene::Scene> texturedRoom(float scale, std::uint32_t side, std::uint32_t samples, const std::string & image);

};  // namespace v3d::moya::bench
