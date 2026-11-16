/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "RIBExportVisitor.h"

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Triangulation.h>
#include <api/brep/Vertex.h>
#include <api/curve/Basis.h>
#include <api/curve/Curves.h>
#include <api/rib/Bases.h>
#include <api/rib/Declaration.h>
#include <api/rib/Handler.h>
#include <api/rib/Parameters.h>
#include <api/rib/requests/Basis.h>
#include <api/rib/requests/Curves.h>
#include <api/rib/requests/PointsPolygons.h>
#include <api/rib/requests/ScreenWindow.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Placed.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::render {

namespace {

typedef v3d::rib::Declaration Declaration;
typedef v3d::rib::ParameterList ParameterList;

const Declaration UNIFORM_FLOAT(Declaration::Storage::UNIFORM, Declaration::Type::FLOAT, 1);
const Declaration UNIFORM_POINT(Declaration::Storage::UNIFORM, Declaration::Type::POINT, 1);

};  // namespace

/**
 **/
std::vector<float> RIBExportVisitor::floats(const glm::vec3& value) {
    return { value.x, value.y, value.z };
}

RIBExportVisitor::RIBExportVisitor(v3d::rib::Handler* handler) : handler_(handler) {
}

void RIBExportVisitor::begin(const v3d::type::camera::Camera& camera, const Output& output) {
    const v3d::type::camera::Profile& profile = camera.profile();
    const glm::vec2 clipping = profile.clipping();
    const scene::RenderSettings& settings = output.settings;
    eye_ = profile.eye();
    looks_ = profile.direction();

    handler_->version(RIB_VERSION);
    handler_->format({.width = settings.size.x, .height = settings.size.y, .pixelAspect = 1.0F});
    handler_->pixelSamples(glm::uvec2(settings.samples));
    if (output.file.empty()) {
        handler_->display({.name = FRAMEBUFFER, .type = "framebuffer", .mode = "rgb"}, ParameterList());
    } else {
        handler_->display({.name = output.file, .type = "file", .mode = "rgb"}, ParameterList());
    }

    /*
        The screen window is sent rather than left to the frame aspect. The editor's camera
        states its aperture as a pixel aspect and an ortho zoom, and a renderer should not have
        to derive the window from those.
    */
    if (profile.orthographic()) {
        handler_->projection("orthographic", ParameterList());
    } else {
        ParameterList fov;
        fov.add("fov", UNIFORM_FLOAT, { profile.fov() }, {});
        handler_->projection("perspective", fov);
    }
    handler_->screenWindow(screen(profile));
    handler_->clipping({.hither = clipping.x, .yon = clipping.y});

    // the transform set between Projection and WorldBegin is the world to camera
    // transformation, which a view matrix already is
    handler_->transform(camera.view());  // NOLINT(build/include_what_you_use)

    handler_->worldBegin();
    handler_->surface(SURFACE, ParameterList());
}

void RIBExportVisitor::end() {
    // the lights were sent as they were visited, so every mesh comes after every light
    if (lights_ == 0) {
        ParameterList light;
        light.add("intensity", UNIFORM_FLOAT, { 1.0F }, {});
        light.add("from", UNIFORM_POINT, floats(eye_), {});
        light.add("to", UNIFORM_POINT, floats(eye_ + looks_), {});
        handler_->attribute("identifier", identifier(CAMERA_LIGHT));
        handler_->lightSource({.shader = "distantlight", .handle = "1"}, light);
    }
    body_.replay(handler_);
    handler_->worldEnd();
}

void RIBExportVisitor::enter(const std::string& name, const glm::mat4x4& local) {
    blocks_.push_back({ .name = name, .local = local });
}

v3d::rib::requests::ScreenWindow RIBExportVisitor::screen(const v3d::type::camera::Profile& profile) {
    if (profile.orthographic()) {
        const float top = profile.orthoZoom();
        const float right = top * profile.pixelAspect();
        return {.left = -right, .right = right, .bottom = -top, .top = top};
    }
    return {.left = -profile.pixelAspect(), .right = profile.pixelAspect(), .bottom = -1.0F, .top = 1.0F};
}

ParameterList RIBExportVisitor::identifier(const std::string& name) {
    ParameterList list;
    list.add("name", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::STRING, 1), {}, { name });
    return list;
}

void RIBExportVisitor::open() {
    for (Block& block : blocks_) {
        if (!block.written) {
            body_.attributeBegin();
            body_.attribute("identifier", identifier(block.name));
            body_.concatTransform(block.local);
            block.written = true;
        }
    }
}

void RIBExportVisitor::leave(const boost::shared_ptr<scene::item::Item>& /*item*/) {
    if (blocks_.empty()) {
        return;
    }
    if (blocks_.back().written) {
        body_.attributeEnd();
    }
    blocks_.pop_back();
}

void RIBExportVisitor::visit(const boost::shared_ptr<scene::item::Mesh>& item, const scene::Placed& placed) {
    // the scene calls leave() for every item it visits, so every visit enters a block
    enter(item ? item->name() : "", placed.local);
    if (!item) {
        return;
    }
    open();
    const boost::shared_ptr<v3d::brep::BRep>& mesh = item->geometry();

    // RI draws a polygon only when it is convex and flat, which a face with more than three
    // corners need not be, so every face is sent as the triangles it is cut into. Those wind
    // the other way from the face's loop, and are turned back to wind as the loop does
    v3d::rib::requests::PointsPolygons faces;
    for (v3d::brep::Index face = 0; face < mesh->faceCount(); face++) {
        for (const std::array<v3d::brep::Index, 3>& triangle : v3d::brep::triangulateFace(*mesh, face)) {
            faces.counts.push_back(3);
            for (const v3d::brep::Index corner : { triangle[0], triangle[2], triangle[1] }) {
                faces.indices.push_back(static_cast<unsigned int>(corner));
            }
        }
    }
    if (faces.counts.empty()) {
        return;
    }
    std::vector<float> points;
    points.reserve(mesh->vertexCount() * 3);
    for (v3d::brep::Index vertex = 0; vertex < mesh->vertexCount(); vertex++) {
        const glm::vec3 point = mesh->vertex(vertex)->point();
        points.insert(points.end(), { point.x, point.y, point.z });
    }
    ParameterList list;
    list.add("P", Declaration(Declaration::Storage::VERTEX, Declaration::Type::POINT, 1), points, {});
    body_.pointsPolygons(faces, list);
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<scene::item::Camera>& camera, const scene::Placed& placed, const scene::item::Camera::Lens& /*lens*/) {
    enter(camera ? camera->name() : "", placed.local);
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<scene::item::Group>& group, const scene::Placed& placed) {
    enter(group ? group->name() : "", placed.local);
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<scene::item::Curve>& curve, const scene::Placed& placed) {
    enter(curve ? curve->name() : "", placed.local);
    if (!curve || !curve->style().renders || curve->geometry()->empty()) {
        return;
    }
    open();

    // RI evaluates every curve of one Curves request through one basis and one wrap, so the
    // curves are sent in runs that share both
    const v3d::curve::Curves& geometry = *curve->geometry();
    std::vector<v3d::curve::BasisCurve> run;
    const auto send = [this, &run, &curve]() {
        if (run.empty()) {
            return;
        }
        const v3d::curve::BasisCurve& first = run.front();
        if (first.basis != v3d::curve::Basis::Linear) {
            v3d::rib::requests::Basis basis;
            v3d::rib::namedBasis(first.basis == v3d::curve::Basis::Bezier ? "bezier" : "catmull-rom", &basis.v);
            basis.vstep = first.basis == v3d::curve::Basis::Bezier ? v3d::rib::requests::Basis::BEZIER_STEP : 1;
            body_.basis(basis);
        }
        v3d::rib::requests::Curves request{ .cubic = first.basis != v3d::curve::Basis::Linear, .periodic = first.periodic };
        std::vector<float> points;
        for (const v3d::curve::BasisCurve& each : run) {
            request.vertices.push_back(static_cast<unsigned int>(each.points.size()));
            for (const glm::vec3& point : each.points) {
                points.insert(points.end(), { point.x, point.y, point.z });
            }
        }
        ParameterList list;
        list.add("P", Declaration(Declaration::Storage::VERTEX, Declaration::Type::POINT, 1), points, {});
        list.add("constantwidth", Declaration(Declaration::Storage::CONSTANT, Declaration::Type::FLOAT, 1), { curve->style().width }, {});
        body_.curves(request, list);
        run.clear();
    };
    for (std::size_t index = 0; index < geometry.size(); index++) {
        v3d::curve::BasisCurve converted = v3d::curve::toBasis(geometry.curve(index), APPROXIMATION_PIECES);
        if (!run.empty() && (run.front().basis != converted.basis || run.front().periodic != converted.periodic)) {
            send();
        }
        run.push_back(std::move(converted));
    }
    send();
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<scene::item::Light>& light, const scene::Placed& placed, const scene::item::Light::Emission& emission) {
    lights_++;
    enter(light ? light->name() : "", placed.local);
    if (!light) {
        return;
    }
    // a light is not scaled, so its direction is its world rotation alone
    const glm::vec3 from(placed.world[3]);
    const glm::vec3 to = from + (scene::orientation(placed.world) * glm::vec3(0.0F, 0.0F, 1.0F));

    std::string shader;
    switch (emission.type) {
        case scene::item::Light::Type::Point:
            shader = "pointlight";
            break;
        case scene::item::Light::Type::Distant:
            shader = "distantlight";
            break;
        case scene::item::Light::Type::Spot:
            shader = "spotlight";
            break;
    }
    ParameterList list;
    list.add("intensity", UNIFORM_FLOAT, { emission.intensity }, {});
    list.add("lightcolor", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::COLOR, 1), floats(emission.colour), {});
    list.add("from", UNIFORM_POINT, floats(from), {});
    if (emission.type != scene::item::Light::Type::Point) {
        list.add("to", UNIFORM_POINT, floats(to), {});
    }
    if (emission.type == scene::item::Light::Type::Spot) {
        list.add("coneangle", UNIFORM_FLOAT, { emission.cone }, {});
        list.add("conedeltaangle", UNIFORM_FLOAT, { emission.delta }, {});
    }
    // a light is written at world level, so its name is set there and holds until the next one
    handler_->attribute("identifier", identifier(light->name()));
    handler_->lightSource({.shader = shader, .handle = std::to_string(lights_)}, list);
}

};  // namespace v3d::editor::render
