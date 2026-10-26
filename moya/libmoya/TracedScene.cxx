/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TracedScene.h"

#include <api/render/offline/MovingTransform.h>
#include <api/render/offline/rib/Requests.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Sphere.h>
#include <api/render/offline/trace/Triangle.h>
#include <moya/libmoya/reyes/GridShader.h>
#include <moya/libmoya/reyes/MicroPolygonGrid.h>

#include <array>
#include <cstddef>

#include <boost/shared_ptr.hpp>
#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Polygon.h"
#include "Shading.h"
#include "Vertex.h"

namespace v3d::moya {

v3d::render::offline::trace::Scene & TracedScene::scene() {
    return scene_;
}

const v3d::render::offline::trace::Scene & TracedScene::scene() const {
    return scene_;
}

void TracedScene::add(const Polygon & poly, const Shading & state, const Submission & with) {
    if (poly.vertexCount() < 3) {
        return;
    }
    if (state.displacement.shader) {
        addDisplaced(poly, state, with);
        return;
    }
    const glm::mat4x4 & toWorld = with.transform->reference();
    const glm::mat3 toWorldNormal = glm::transpose(glm::inverse(glm::mat3(toWorld)));

    // a fan, since RI says a polygon is planar and convex
    const Vertex first = poly.vertex(0);
    for (std::size_t i = 1; i + 1 < poly.vertexCount(); i++) {
        const Vertex corners[3] = { first, poly.vertex(i), poly.vertex(i + 1) };
        v3d::render::offline::trace::Triangle::Corners points;
        v3d::render::offline::trace::Triangle::Corners normals;
        for (std::size_t k = 0; k < 3; k++) {
            points[k] = glm::vec3(toWorld * glm::vec4(corners[k].point(), 1.0F));
            normals[k] = corners[k].hasNormal() ? glm::normalize(toWorldNormal * corners[k].normal()) : glm::vec3(0.0F);
        }
        const bool shaded = corners[0].hasNormal() && corners[1].hasNormal() && corners[2].hasNormal();
        v3d::render::offline::trace::Triangle triangle = shaded ?
            v3d::render::offline::trace::Triangle(points, with.colour, normals) :
            v3d::render::offline::trace::Triangle(points, with.colour);
        if (corners[0].hasTexCoord() && corners[1].hasTexCoord() && corners[2].hasTexCoord()) {
            triangle.st(corners[0].st(), corners[1].st(), corners[2].st());
        }
        if (corners[0].hasColor() && corners[1].hasColor() && corners[2].hasColor()) {
            triangle.colours(corners[0].color(), corners[1].color(), corners[2].color());
        }
        shade(&triangle, state, with);
        scene_.add(triangle, *with.transform);
        if (with.area != nullptr) {
            with.area->add(points[0], points[1], points[2]);
        }
    }
}

void TracedScene::addDisplaced(const Polygon & poly, const Shading & state, const Submission & with) {
    // a copy in camera space, where the displacement shader runs, with each vertex given the
    // plane and the colour the reyes hider would give it
    const glm::mat4x4 toCamera = with.view * with.transform->reference();
    const glm::mat3 toCameraNormal = glm::transpose(glm::inverse(glm::mat3(toCamera)));
    Polygon eye;
    for (std::size_t i = 0; i < poly.vertexCount(); i++) {
        Vertex corner = poly.vertex(i);
        corner.point(glm::vec3(toCamera * glm::vec4(corner.point(), 1.0F)));
        if (corner.hasNormal()) {
            corner.normal(glm::normalize(toCameraNormal * corner.normal()));
        }
        eye.addVertex(corner);
    }
    const glm::vec3 plane = eye.geometricNormal();
    for (std::size_t i = 0; i < eye.vertexCount(); i++) {
        eye[i].geometricNormal(plane);
        if (!eye[i].hasNormal()) {
            eye[i].normal(plane);
        }
        if (!eye[i].hasColor()) {
            eye[i].color(with.colour);
        }
    }
    boost::shared_ptr<reyes::MicroPolygonGrid> grid;
    if (!eye.dice(grid, tracedAcross)) {
        return;
    }
    with.displacer->displace(state, grid.get());

    const glm::mat4x4 toWorld = glm::inverse(with.view);
    const glm::mat3 toWorldNormal = glm::transpose(glm::inverse(glm::mat3(toWorld)));
    const unsigned int size = grid->size();
    const float span = static_cast<float>(size - 1);
    const auto corner = [&](unsigned int alongU, unsigned int alongV) {
        Vertex at = grid->vertex(alongU, alongV);
        at.point(glm::vec3(toWorld * glm::vec4(at.point(), 1.0F)));
        at.normal(glm::normalize(toWorldNormal * at.normal()));
        // a grid without st takes its own parameters as s and t, as a shaded grid does
        if (!at.hasTexCoord()) {
            at.st(glm::vec2(static_cast<float>(alongU) / span, static_cast<float>(alongV) / span));
        }
        return at;
    };
    for (unsigned int i = 0; i + 1 < size; i++) {
        for (unsigned int j = 0; j + 1 < size; j++) {
            const Vertex quad[4] = { corner(i, j), corner(i + 1, j), corner(i + 1, j + 1), corner(i, j + 1) };
            for (const std::array<int, 3> & fan : { std::array<int, 3>{0, 1, 2}, std::array<int, 3>{0, 2, 3} }) {
                const Vertex & first = quad[fan[0]];
                const Vertex & second = quad[fan[1]];
                const Vertex & third = quad[fan[2]];
                v3d::render::offline::trace::Triangle triangle({first.point(), second.point(), third.point()}, with.colour,
                    {first.normal(), second.normal(), third.normal()});
                triangle.st(first.st(), second.st(), third.st());
                triangle.colours(first.color(), second.color(), third.color());
                shade(&triangle, state, with);
                scene_.add(triangle, *with.transform);
            }
        }
    }
}

void TracedScene::shade(v3d::render::offline::trace::Primitive* primitive, const Shading & state, const Submission & with) {
    // a shader's placement takes it to camera space, and the traced scene is in world space
    v3d::render::offline::sl::Placed surface;
    surface.shader = state.surface;
    surface.placement = glm::inverse(with.view) * state.placement;
    primitive->surface(surface);
    primitive->object(glm::inverse(with.view) * state.object);
    primitive->opacity(state.opacity);
    primitive->lights(with.lights);
}

void TracedScene::add(const v3d::render::offline::rib::Sphere & shape, const Shading & state, const Submission & with) {
    // placed by the reference end of its motion, as a polygon's points are
    v3d::render::offline::trace::Sphere sphere(shape, with.transform->reference(), with.colour);
    shade(&sphere, state, with);
    scene_.add(sphere, *with.transform);
}

};  // namespace v3d::moya
