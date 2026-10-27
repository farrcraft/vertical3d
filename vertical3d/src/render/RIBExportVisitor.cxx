/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBExportVisitor.h"

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Triangulation.h>
#include <api/brep/Vertex.h>
#include <api/render/offline/rib/Declaration.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/scene/Camera.h>
#include <vertical3d/src/scene/Curve.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Group.h>
#include <vertical3d/src/scene/Item.h>
#include <vertical3d/src/scene/Light.h>
#include <vertical3d/src/scene/Mesh.h>
#include <vertical3d/src/scene/RenderSettings.h>

#include <array>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

#include <glm/gtc/quaternion.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor {

namespace {

typedef v3d::render::offline::rib::Declaration Declaration;
typedef v3d::render::offline::rib::ParameterList ParameterList;

const Declaration UNIFORM_FLOAT(Declaration::Storage::UNIFORM, Declaration::Type::FLOAT, 1);
const Declaration UNIFORM_POINT(Declaration::Storage::UNIFORM, Declaration::Type::POINT, 1);

/** The version of RIB the export is written in. **/
const float RIB_VERSION = 3.03F;

std::vector<float> floats(const glm::vec3& value) {
    return { value.x, value.y, value.z };
}

};  // namespace

RIBExportVisitor::RIBExportVisitor(v3d::render::offline::rib::Handler* handler) : handler_(handler) {
}

void RIBExportVisitor::begin(const v3d::type::camera::Camera& camera, const Output& output) {
    const v3d::type::camera::Profile& profile = camera.profile();
    const glm::vec2 clipping = profile.clipping();
    const RenderSettings& settings = output.settings;
    eye_ = profile.eye();
    looks_ = profile.direction();

    handler_->version(RIB_VERSION);
    handler_->format({.width = settings.size.x, .height = settings.size.y, .pixelAspect = 1.0F});
    handler_->pixelSamples(glm::uvec2(settings.samples));
    handler_->hider(settings.hider == RenderSettings::Hider::RayTrace ? "raytrace" : "hidden", ParameterList());
    if (output.file.empty()) {
        handler_->display({.name = "vertical3d", .type = "framebuffer", .mode = "rgb"}, ParameterList());
    } else {
        handler_->display({.name = output.file, .type = "file", .mode = "rgb"}, ParameterList());
    }

    /*
        The screen window is sent rather than left to the frame aspect. The editor's camera
        states its aperture as a pixel aspect and an ortho zoom, and a renderer should not have
        to derive the window from those.
    */
    if (profile.orthographic()) {
        const float top = profile.orthoZoom();
        const float right = top * profile.pixelAspect();
        handler_->projection("orthographic", ParameterList());
        handler_->screenWindow({.left = -right, .right = right, .bottom = -top, .top = top});
    } else {
        ParameterList fov;
        fov.add("fov", UNIFORM_FLOAT, { profile.fov() }, {});
        handler_->projection("perspective", fov);
        handler_->screenWindow({.left = -profile.pixelAspect(), .right = profile.pixelAspect(), .bottom = -1.0F, .top = 1.0F});
    }
    handler_->clipping({.hither = clipping.x, .yon = clipping.y});

    // the transform set between Projection and WorldBegin is the world to camera
    // transformation, which a view matrix already is
    handler_->transform(camera.view());  // NOLINT(build/include_what_you_use)

    handler_->worldBegin();
    handler_->surface("matte", ParameterList());
}

void RIBExportVisitor::end() {
    // the lights were sent as they were visited, so every mesh comes after every light
    if (lights_ == 0) {
        ParameterList light;
        light.add("intensity", UNIFORM_FLOAT, { 1.0F }, {});
        light.add("from", UNIFORM_POINT, floats(eye_), {});
        light.add("to", UNIFORM_POINT, floats(eye_ + looks_), {});
        handler_->lightSource({.shader = "distantlight", .handle = "1"}, light);
    }
    body_.replay(handler_);
    handler_->worldEnd();
}

void RIBExportVisitor::enter(const std::string& name, const glm::mat4x4& local) {
    blocks_.push_back({ .name = name, .local = local });
}

void RIBExportVisitor::open() {
    for (Block& block : blocks_) {
        if (!block.written) {
            ParameterList identifier;
            identifier.add("name", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::STRING, 1), {}, { block.name });
            body_.attributeBegin();
            body_.attribute("identifier", identifier);
            body_.concatTransform(block.local);
            block.written = true;
        }
    }
}

void RIBExportVisitor::leave(const boost::shared_ptr<Item>& /*item*/) {
    if (blocks_.empty()) {
        return;
    }
    if (blocks_.back().written) {
        body_.attributeEnd();
    }
    blocks_.pop_back();
}

void RIBExportVisitor::visit(const boost::shared_ptr<Mesh>& item, const Placed& placed) {
    // the scene calls leave() for every item it visits, so every visit enters a block
    meshes_++;
    enter("mesh" + std::to_string(meshes_), placed.local);
    if (!item) {
        return;
    }
    open();
    const boost::shared_ptr<v3d::brep::BRep>& mesh = item->geometry();

    // RI draws a Polygon only when it is convex and flat, which a face with more than three
    // corners need not be, so every face is sent as the triangles it is cut into. Those wind
    // the other way from the face's loop, and are turned back to wind as the loop does
    for (v3d::brep::Index face = 0; face < mesh->faceCount(); face++) {
        for (const std::array<v3d::brep::Index, 3>& triangle : v3d::brep::triangulateFace(*mesh, face)) {
            std::vector<float> points;
            for (const v3d::brep::Index corner : { triangle[0], triangle[2], triangle[1] }) {
                const glm::vec3 point = mesh->vertex(corner)->point();
                points.insert(points.end(), { point.x, point.y, point.z });
            }
            ParameterList list;
            list.add("P", Declaration(Declaration::Storage::VERTEX, Declaration::Type::POINT, 1), points, {});
            body_.polygon(3, list);
        }
    }
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<Camera>& /*camera*/, const Placed& placed, const Camera::Lens& /*lens*/) {
    cameras_++;
    enter("camera" + std::to_string(cameras_), placed.local);
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<Group>& /*group*/, const Placed& placed) {
    groups_++;
    enter("group" + std::to_string(groups_), placed.local);
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<Curve>& /*curve*/, const Placed& placed) {
    curves_++;
    enter("curve" + std::to_string(curves_), placed.local);
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<Light>& light, const Placed& placed, const Light::Emission& emission) {
    lights_++;
    enter("light" + std::to_string(lights_), placed.local);
    if (!light) {
        return;
    }
    // a light is not scaled, so its direction is its world rotation alone
    const glm::vec3 from(placed.world[3]);
    const glm::vec3 to = from + (orientation(placed.world) * glm::vec3(0.0F, 0.0F, 1.0F));

    std::string shader;
    switch (emission.type) {
        case Light::Type::Point:
            shader = "pointlight";
            break;
        case Light::Type::Distant:
            shader = "distantlight";
            break;
        case Light::Type::Spot:
            shader = "spotlight";
            break;
    }
    ParameterList list;
    list.add("intensity", UNIFORM_FLOAT, { emission.intensity }, {});
    list.add("lightcolor", Declaration(Declaration::Storage::UNIFORM, Declaration::Type::COLOR, 1), floats(emission.colour), {});
    list.add("from", UNIFORM_POINT, floats(from), {});
    if (emission.type != Light::Type::Point) {
        list.add("to", UNIFORM_POINT, floats(to), {});
    }
    if (emission.type == Light::Type::Spot) {
        list.add("coneangle", UNIFORM_FLOAT, { emission.cone }, {});
        list.add("conedeltaangle", UNIFORM_FLOAT, { emission.delta }, {});
    }
    handler_->lightSource({.shader = shader, .handle = std::to_string(lights_)}, list);
}

};  // namespace v3d::editor
