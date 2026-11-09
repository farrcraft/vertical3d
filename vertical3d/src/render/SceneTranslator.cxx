/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "SceneTranslator.h"

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/brep/Triangulation.h>
#include <api/brep/Vertex.h>
#include <api/curve/Basis.h>
#include <api/curve/Curves.h>
#include <api/render/offline/rib/requests/ScreenWindow.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kind.h>
#include <moya/libmoya/scene/Placement.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>
#include <vertical3d/src/scene/Evaluation.h>
#include <vertical3d/src/scene/Placed.h>
#include <vertical3d/src/scene/item/Camera.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/Group.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Light.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "RIBExportVisitor.h"
#include "Shot.h"

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec3.hpp>

namespace v3d::editor::render {

namespace {

namespace moya = v3d::moya::scene;

};  // namespace

SceneTranslator::Walk::Walk(SceneTranslator* translator) : translator_(translator) {
}

unsigned int SceneTranslator::Walk::lights() const {
    return lights_;
}

void SceneTranslator::Walk::visit(const boost::shared_ptr<scene::item::Mesh>& item, const scene::Placed& placed) {
    bases_.push_back(bases_.back());
    if (!item) {
        return;
    }
    // the triangles of each face, as the export cuts them and winds them
    const v3d::brep::BRep& mesh = *item->geometry();
    std::vector<std::int32_t> counts;
    std::vector<std::int32_t> indices;
    for (v3d::brep::Index face = 0; face < mesh.faceCount(); face++) {
        for (const std::array<v3d::brep::Index, 3>& triangle : v3d::brep::triangulateFace(mesh, face)) {
            counts.push_back(3);
            for (const v3d::brep::Index corner : { triangle[0], triangle[2], triangle[1] }) {
                indices.push_back(static_cast<std::int32_t>(corner));
            }
        }
    }
    if (counts.empty()) {
        return;
    }
    std::vector<glm::vec3> points;
    points.reserve(mesh.vertexCount());
    for (v3d::brep::Index vertex = 0; vertex < mesh.vertexCount(); vertex++) {
        points.push_back(mesh.vertex(vertex)->point());
    }
    const moya::Handle prototype = translator_->object("mesh", item->name());
    translator_->put(prototype, "P", moya::Value::points(points));
    translator_->put(prototype, "vertices", moya::Value::integers(counts));
    translator_->put(prototype, "indices", moya::Value::integers(indices));
    place(prototype, item->name(), placed);
}

void SceneTranslator::Walk::visit(const boost::shared_ptr<scene::item::Camera>& camera, const scene::Placed& placed, const scene::item::Camera::Lens& lens) {
    (void)camera;
    (void)placed;
    (void)lens;
    bases_.push_back(bases_.back());
}

void SceneTranslator::Walk::visit(const boost::shared_ptr<scene::item::Group>& group, const scene::Placed& placed) {
    (void)group;
    (void)placed;
    bases_.push_back(bases_.back());
}

void SceneTranslator::Walk::visit(const boost::shared_ptr<scene::item::Curve>& curve, const scene::Placed& placed) {
    bases_.push_back(bases_.back());
    if (!curve || !curve->style().renders || curve->geometry()->empty()) {
        return;
    }
    // the curves in runs that share a basis and a wrap, as RI evaluates one request through one
    const v3d::curve::Curves& geometry = *curve->geometry();
    std::vector<std::vector<v3d::curve::BasisCurve>> runs;
    for (std::size_t index = 0; index < geometry.size(); index++) {
        v3d::curve::BasisCurve converted = v3d::curve::toBasis(geometry.curve(index), RIBExportVisitor::APPROXIMATION_PIECES);
        if (runs.empty() || runs.back().front().basis != converted.basis || runs.back().front().periodic != converted.periodic) {
            runs.emplace_back();
        }
        runs.back().push_back(std::move(converted));
    }
    for (std::size_t run = 0; run < runs.size(); run++) {
        const std::vector<v3d::curve::BasisCurve>& curves = runs[run];
        const v3d::curve::BasisCurve& first = curves.front();
        // a linear run keeps the basis the block holds, which a cubic run before it set
        if (first.basis != v3d::curve::Basis::Linear) {
            bases_.back() = first.basis == v3d::curve::Basis::Bezier ? "bezier" : "catmull-rom";
        }
        std::vector<std::int32_t> vertices;
        std::vector<glm::vec3> points;
        for (const v3d::curve::BasisCurve& each : curves) {
            vertices.push_back(static_cast<std::int32_t>(each.points.size()));
            points.insert(points.end(), each.points.begin(), each.points.end());
        }
        const std::string name = run == 0 ? curve->name() : curve->name() + " " + std::to_string(run + 1);
        const moya::Handle prototype = translator_->object("curves", name);
        translator_->put(prototype, "P", moya::Value::points(points));
        translator_->put(prototype, "vertices", moya::Value::integers(vertices));
        translator_->put(prototype, "type", moya::Value::string(first.basis == v3d::curve::Basis::Linear ? "linear" : "cubic"));
        translator_->put(prototype, "wrap", moya::Value::string(first.periodic ? "periodic" : "nonperiodic"));
        translator_->put(prototype, "basis", moya::Value::string(bases_.back()));
        translator_->put(prototype, "constantWidth", moya::Value::number(curve->style().width));
        place(prototype, name, placed);
    }
}

void SceneTranslator::Walk::visit(const boost::shared_ptr<scene::item::Light>& light, const scene::Placed& placed, const scene::item::Light::Emission& emission) {
    bases_.push_back(bases_.back());
    if (!light) {
        return;
    }
    lights_++;
    // a light is not scaled, so its direction is its world rotation alone
    // a point light has no direction, so it looks down z from where it is, as RI's does
    const glm::vec3 from(placed.world[3]);
    const glm::vec3 to = from + (emission.type == scene::item::Light::Type::Point ? glm::vec3(0.0F, 0.0F, 1.0F) :
        scene::orientation(placed.world) * glm::vec3(0.0F, 0.0F, 1.0F));
    const std::string kind = emission.type == scene::item::Light::Type::Distant ? "distantLight" : "sphereLight";
    const moya::Handle made = translator_->object(kind, light->name());
    translator_->put(made, "transform", moya::Value::matrix(moya::aimed(from, to)));
    translator_->put(made, "colour", moya::Value::colour(emission.colour));
    translator_->put(made, "intensity", moya::Value::number(emission.intensity));
    if (emission.type == scene::item::Light::Type::Distant) {
        translator_->put(made, "angle", moya::Value::number(0.0F));
        return;
    }
    translator_->put(made, "radius", moya::Value::number(0.0F));
    // a point light shares a spot light's kind, so a light that stops being a spot loses its cone
    if (emission.type == scene::item::Light::Type::Spot) {
        translator_->put(made, "coneAngle", moya::Value::number(glm::degrees(emission.cone)));
        translator_->put(made, "coneSoftness", moya::Value::number(emission.cone > 0.0F ? std::clamp(emission.delta / emission.cone, 0.0F, 1.0F) : 0.0F));
    } else {
        translator_->reset(made, "coneAngle");
        translator_->reset(made, "coneSoftness");
    }
}

void SceneTranslator::Walk::leave(const boost::shared_ptr<scene::item::Item>& item) {
    (void)item;
    if (bases_.size() > 1) {
        bases_.pop_back();
    }
}

void SceneTranslator::Walk::place(const moya::Handle& prototype, const std::string& name, const scene::Placed& placed) {
    const moya::Handle instance = translator_->object("instance", name);
    translator_->put(instance, "prototype", moya::Value::handle(prototype));
    translator_->put(instance, "transform", moya::Value::matrix(placed.world));
    translator_->put(instance, "material", moya::Value::handle(translator_->material()));
}

SceneTranslator::SceneTranslator(v3d::moya::scene::Scene* scene) : scene_(scene) {
}

v3d::moya::scene::Handle SceneTranslator::object(const std::string& kind, const std::string& name) {
    const std::pair<std::string, std::string> key(kind, name);
    seen_.insert(key);
    const auto found = objects_.find(key);
    if (found != objects_.end()) {
        return found->second;
    }
    const moya::Handle made = scene_->create(kind, name);
    objects_[key] = made;
    changed_ = true;
    return made;
}

void SceneTranslator::reset(const v3d::moya::scene::Handle& object, const std::string& parameter) {
    const auto held = values_.find({ object.index, object.generation });
    if (held == values_.end() || !held->second.contains(parameter)) {
        return;
    }
    for (const moya::Declaration& declared : scene_->schema().kind(object.kind).parameters) {
        if (declared.name == parameter) {
            put(object, parameter, declared.fallback);
            return;
        }
    }
}

void SceneTranslator::put(const v3d::moya::scene::Handle& object, const std::string& parameter, const v3d::moya::scene::Value& value) {
    std::map<std::string, moya::Value>& values = values_[{ object.index, object.generation }];
    const auto found = values.find(parameter);
    if (found != values.end() && found->second == value) {
        return;
    }
    scene_->set(object, parameter, value);
    values[parameter] = value;
    changed_ = true;
}

v3d::moya::scene::Handle SceneTranslator::material() {
    const bool made = !objects_.contains({ "shader", RIBExportVisitor::SURFACE });
    const moya::Handle surface = object("shader", RIBExportVisitor::SURFACE);
    if (made) {
        // the colour and opacity RI's state gives a surface that names neither
        for (const char* input : { "Cs", "Os" }) {
            scene_->declare(surface, moya::Declaration { .name = input, .type = moya::Type::Colour, .fallback = moya::Value::colour(glm::vec3(1.0F)) });
        }
    }
    put(surface, "shader", moya::Value::string(RIBExportVisitor::SURFACE));
    put(surface, "language", moya::Value::string("ri"));
    const moya::Handle material = object("material", RIBExportVisitor::SURFACE);
    put(material, "surface", moya::Value::handle(surface));
    return material;
}

bool SceneTranslator::update(const scene::Scene& scene, const scene::Evaluation& evaluation, const v3d::type::camera::Profile& from,
    const RIBExportVisitor::Output& output) {
    seen_.clear();
    changed_ = false;

    // the frame, as the export sends it
    const v3d::type::camera::Camera camera = shotCamera(from, output.settings);
    const v3d::type::camera::Profile& profile = camera.profile();
    const moya::Handle options = object("options", "options");
    const moya::Handle lens = object("camera", "camera");
    put(options, "camera", moya::Value::handle(lens));
    put(options, "resolution", moya::Value::integers({ static_cast<std::int32_t>(output.settings.size.x), static_cast<std::int32_t>(output.settings.size.y) }));
    put(options, "pixelAspect", moya::Value::number(1.0F));
    put(options, "samples", moya::Value::integer(static_cast<std::int32_t>(output.settings.samples * output.settings.samples)));
    put(lens, "projection", moya::Value::string(profile.orthographic() ? "orthographic" : "perspective"));
    // an orthographic camera has no field of view, so one that turns orthographic loses it
    if (profile.orthographic()) {
        reset(lens, "fov");
    } else {
        put(lens, "fov", moya::Value::number(profile.fov()));
    }
    const v3d::render::offline::rib::requests::ScreenWindow screen = RIBExportVisitor::screen(profile);
    put(lens, "screen", moya::Value::floats({ screen.left, screen.right, screen.bottom, screen.top }));
    put(lens, "near", moya::Value::number(profile.clipping().x));
    put(lens, "far", moya::Value::number(profile.clipping().y));
    put(lens, "transform", moya::Value::matrix(glm::inverse(camera.view())));
    const std::string file = output.file.empty() ? RIBExportVisitor::FRAMEBUFFER : output.file;
    const moya::Handle written = object("output", file);
    put(written, "file", moya::Value::string(file));
    std::string format = file.ends_with(".png") ? "png" : "exr";
    if (output.file.empty()) {
        format = "framebuffer";
    }
    put(written, "format", moya::Value::string(format));
    put(written, "channels", moya::Value::handles({}));

    Walk walk(this);
    scene.accept(&walk, evaluation, scene::Scene::Showing::Renders);
    if (walk.lights() == 0) {
        const glm::vec3 eye = profile.eye();
        const moya::Handle light = object("distantLight", RIBExportVisitor::CAMERA_LIGHT);
        put(light, "transform", moya::Value::matrix(moya::aimed(eye, eye + profile.direction())));
        put(light, "colour", moya::Value::colour(glm::vec3(1.0F)));
        put(light, "intensity", moya::Value::number(1.0F));
        put(light, "angle", moya::Value::number(0.0F));
    }

    // what no item made this time is gone
    for (auto at = objects_.begin(); at != objects_.end();) {
        if (seen_.contains(at->first)) {
            ++at;
            continue;
        }
        values_.erase({ at->second.index, at->second.generation });
        scene_->remove(at->second);
        at = objects_.erase(at);
        changed_ = true;
    }
    if (changed_) {
        scene_->commit();
    }
    return changed_;
}

};  // namespace v3d::editor::render
