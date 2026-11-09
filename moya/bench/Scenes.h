/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/scene/Scene.h>

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

};  // namespace v3d::moya::bench
