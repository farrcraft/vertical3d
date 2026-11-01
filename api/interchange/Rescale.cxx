/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Rescale.h"

#include <api/curve/Curve.h>
#include <api/interchange/Scene.h>

#include <variant>
#include <vector>

#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace v3d::interchange {

namespace {

void scalePoints(std::vector<glm::vec3>* points, float factor) {
    for (glm::vec3& point : *points) {
        point *= factor;
    }
}

void scaleCurve(v3d::curve::Curve* curve, float factor) {
    if (auto* poly = std::get_if<v3d::curve::Poly>(&curve->shape)) {
        scalePoints(&poly->points, factor);
    } else if (auto* bezier = std::get_if<v3d::curve::Bezier>(&curve->shape)) {
        for (v3d::curve::BezierPoint& point : bezier->points) {
            point.position *= factor;
            point.left *= factor;
            point.right *= factor;
        }
    } else if (auto* smooth = std::get_if<v3d::curve::CatmullRom>(&curve->shape)) {
        scalePoints(&smooth->points, factor);
    } else if (auto* nurbs = std::get_if<v3d::curve::Nurbs>(&curve->shape)) {
        scalePoints(&nurbs->points, factor);
    }
}

void scaleValues(std::vector<glm::vec4>* values, float factor) {
    // only x, y and z are a translation's; w is unused
    for (glm::vec4& value : *values) {
        value = glm::vec4(glm::vec3(value) * factor, value.w);
    }
}

};  // namespace

void rescale(Scene* scene, float factor) {
    if (scene == nullptr) {
        return;
    }
    for (Mesh& mesh : scene->meshes) {
        scalePoints(&mesh.points, factor);
    }
    for (Node& node : scene->nodes) {
        node.placement.position *= factor;
    }
    for (Channel& channel : scene->channels) {
        if (channel.path == Channel::Path::Translation) {
            scaleValues(&channel.values, factor);
            scaleValues(&channel.inTangents, factor);
            scaleValues(&channel.outTangents, factor);
        }
    }
    for (Curves& curves : scene->curves) {
        for (v3d::curve::Curve& curve : curves.curves) {
            scaleCurve(&curve, factor);
        }
        curves.width *= factor;
    }
    for (Camera& camera : scene->cameras) {
        camera.clipping *= factor;
        camera.magnification *= factor;
    }
}

};  // namespace v3d::interchange
