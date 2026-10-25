/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "RIBExportVisitor.h"

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Topology.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>

#include <cstddef>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <vector>

#include "Camera.h"
#include "Evaluation.h"
#include "Group.h"
#include "Item.h"
#include "Light.h"
#include "Mesh.h"

#include <glm/gtc/quaternion.hpp>
#include <glm/vec3.hpp>


namespace v3d::editor {

RIBExportVisitor::RIBExportVisitor(std::ostream* stream) : stream_(stream) {
}

void RIBExportVisitor::matrix(std::ostream* out, const glm::mat4x4& transform) {
    *out << "[";
    for (int column = 0; column < 4; column++) {
        for (int row = 0; row < 4; row++) {
            *out << " " << transform[column][row];
        }
    }
    *out << " ]";
}

void RIBExportVisitor::begin(const v3d::type::camera::Camera& camera, unsigned int width, unsigned int height) {
    const v3d::type::camera::Profile& profile = camera.profile();
    const glm::vec2 clipping = profile.clipping();

    *stream_ << "##RenderMan RIB-Structure 1.1\n";
    *stream_ << "##Creator Vertical3D\n";
    *stream_ << "version 3.03\n";
    *stream_ << "Format " << width << " " << height << " 1\n";

    /*
        The screen window is written rather than left to the frame aspect. The editor's
        camera states its aperture as a pixel aspect and an ortho zoom, and a reader should
        not have to derive the window from those.
    */
    if (profile.orthographic()) {
        const float top = profile.orthoZoom();
        const float right = top * profile.pixelAspect();
        *stream_ << "Projection \"orthographic\"\n";
        *stream_ << "ScreenWindow " << -right << " " << right << " " << -top << " " << top << "\n";
    } else {
        *stream_ << "Projection \"perspective\" \"fov\" [" << profile.fov() << "]\n";
        *stream_ << "ScreenWindow " << -profile.pixelAspect() << " " << profile.pixelAspect() << " -1 1\n";
    }
    *stream_ << "Clipping " << clipping.x << " " << clipping.y << "\n";

    // the transform set between Projection and WorldBegin is the world to camera
    // transformation, which a view matrix already is
    *stream_ << "Transform ";
    matrix(stream_, camera.view());
    *stream_ << "\n";

    *stream_ << "WorldBegin\n";
}

void RIBExportVisitor::end() {
    // the lights were written as they were visited, so every mesh comes after every light
    *stream_ << body_.str();
    body_.str(std::string());
    *stream_ << "WorldEnd\n";
}

void RIBExportVisitor::enter(const std::string& name, const glm::mat4x4& local) {
    std::ostringstream begin;
    begin << "AttributeBegin\n";
    begin << "Attribute \"identifier\" \"name\" [\"" << name << "\"]\n";
    begin << "ConcatTransform ";
    matrix(&begin, local);
    begin << "\n";
    blocks_.push_back({ .begin = begin.str() });
}

void RIBExportVisitor::open() {
    for (Block& block : blocks_) {
        if (!block.written) {
            body_ << block.begin;
            block.written = true;
        }
    }
}

void RIBExportVisitor::leave(const boost::shared_ptr<Item>& /*item*/) {
    if (blocks_.empty()) {
        return;
    }
    if (blocks_.back().written) {
        body_ << "AttributeEnd\n";
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

    for (v3d::brep::Index face = 0; face < mesh->faceCount(); face++) {
        const std::vector<v3d::brep::Index> loop = v3d::brep::faceLoop(*mesh, face);
        if (loop.size() < 3) {
            continue;
        }
        // a half edge names the vertex it ends at, so each segment starts where the one
        // before it ended. Taking each segment's start visits every vertex of the face once
        std::vector<glm::vec3> points;
        for (std::size_t index = 0; index < loop.size(); index++) {
            const std::optional<v3d::brep::Segment> segment = v3d::brep::loopSegment(*mesh, loop, index);
            if (!segment) {
                points.clear();
                break;
            }
            points.push_back(segment->from);
        }
        if (points.size() < 3) {
            continue;
        }

        body_ << "Polygon \"P\" [";
        for (const glm::vec3& point : points) {
            body_ << " " << point.x << " " << point.y << " " << point.z;
        }
        body_ << " ]\n";
    }
}

/**
 **/
void RIBExportVisitor::visit(const boost::shared_ptr<Camera>& /*camera*/, const Placed& placed) {
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
void RIBExportVisitor::visit(const boost::shared_ptr<Light>& light, const Placed& placed) {
    lights_++;
    enter("light" + std::to_string(lights_), placed.local);
    if (!light) {
        return;
    }
    const Light::Emission& emission = light->emission();
    // a light is not scaled, so its direction is its world rotation alone
    const glm::vec3 from(placed.world[3]);
    const glm::vec3 to = from + (orientation(placed.world) * glm::vec3(0.0F, 0.0F, 1.0F));
    const auto point = [this](const glm::vec3& value) {
        *stream_ << " [" << value.x << " " << value.y << " " << value.z << "]";
    };

    switch (emission.type) {
        case Light::Type::Point:
            *stream_ << "LightSource \"pointlight\" " << lights_;
            break;
        case Light::Type::Distant:
            *stream_ << "LightSource \"distantlight\" " << lights_;
            break;
        case Light::Type::Spot:
            *stream_ << "LightSource \"spotlight\" " << lights_;
            break;
    }
    *stream_ << " \"intensity\" [" << emission.intensity << "] \"lightcolor\"";
    point(emission.colour);
    *stream_ << " \"from\"";
    point(from);
    if (emission.type != Light::Type::Point) {
        *stream_ << " \"to\"";
        point(to);
    }
    if (emission.type == Light::Type::Spot) {
        *stream_ << " \"coneangle\" [" << emission.cone << "] \"conedeltaangle\" [" << emission.delta << "]";
    }
    *stream_ << "\n";
}

};  // namespace v3d::editor
