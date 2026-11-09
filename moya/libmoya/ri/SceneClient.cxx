/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "SceneClient.h"

#include <api/log/Logger.h>
#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Bases.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/requests/Basis.h>
#include <api/render/offline/rib/requests/Clipping.h>
#include <api/render/offline/rib/requests/Cone.h>
#include <api/render/offline/rib/requests/CropWindow.h>
#include <api/render/offline/rib/requests/Curves.h>
#include <api/render/offline/rib/requests/Cylinder.h>
#include <api/render/offline/rib/requests/DepthOfField.h>
#include <api/render/offline/rib/requests/Disk.h>
#include <api/render/offline/rib/requests/Display.h>
#include <api/render/offline/rib/requests/Format.h>
#include <api/render/offline/rib/requests/Hyperboloid.h>
#include <api/render/offline/rib/requests/LightSource.h>
#include <api/render/offline/rib/requests/Paraboloid.h>
#include <api/render/offline/rib/requests/PointsPolygons.h>
#include <api/render/offline/rib/requests/ScreenWindow.h>
#include <api/render/offline/rib/requests/Shutter.h>
#include <api/render/offline/rib/requests/Sphere.h>
#include <api/render/offline/rib/requests/SubdivisionMesh.h>
#include <api/render/offline/rib/requests/Torus.h>
#include <moya/libmoya/ri/internal/Primvars.h>
#include <moya/libmoya/ri/internal/Sampled.h>
#include <moya/libmoya/ri/internal/Shape.h>
#include <moya/libmoya/ri/internal/Shapes.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kind.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Schema.h>
#include <moya/libmoya/scene/Value.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/matrix.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::ri {

SceneClient::SceneClient(scene::Scene & scene, const boost::shared_ptr<v3d::log::Logger> & logger) :
    scene_(scene), logger_(logger), warnings_(logger), made_(&scene), lights_(&scene, &made_, &warnings_, logger),
    deformations_(&scene, &warnings_, logger, current_.basis) {
    options_ = scene_.create("options", "options");
    camera_ = scene_.create("camera", "camera");
    scene_.set(options_, "camera", scene::Value::handle(camera_));
}

bool SceneClient::endsWith(const std::string & text, const std::string & suffix) {
    return text.size() >= suffix.size() && text.compare(text.size() - suffix.size(), suffix.size(), suffix) == 0;
}

void SceneClient::frame(const std::function<void()> & committed) {
    committed_ = committed;
}

void SceneClient::format(const v3d::render::offline::rib::requests::Format & request) {
    const int width = request.width == 0 ? kWidth : static_cast<int>(request.width);
    const int height = request.height == 0 ? kHeight : static_cast<int>(request.height);
    scene_.set(options_, "resolution", scene::Value::integers({ width, height }));
    scene_.set(options_, "pixelAspect", scene::Value::number(request.pixelAspect));
}

void SceneClient::frameAspectRatio(float aspect) {
    scene_.set(options_, "frameAspect", scene::Value::number(aspect));
}

void SceneClient::screenWindow(const v3d::render::offline::rib::requests::ScreenWindow & request) {
    scene_.set(camera_, "screen", scene::Value::floats({ request.left, request.right, request.bottom, request.top }));
}

void SceneClient::cropWindow(const v3d::render::offline::rib::requests::CropWindow & request) {
    scene_.set(options_, "crop", scene::Value::floats({ request.xmin, request.xmax, request.ymin, request.ymax }));
}

void SceneClient::projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name != "perspective" && name != "orthographic") {
        logger_->get()->warn("moya's camera is perspective or orthographic, so Projection \"{}\" changes nothing", name);
        return;
    }
    scene_.set(camera_, "projection", scene::Value::string(name));
    if (parameters.has("fov")) {
        scene_.set(camera_, "fov", scene::Value::number(parameters.number("fov", 0.0F)));
    }
}

void SceneClient::clipping(const v3d::render::offline::rib::requests::Clipping & request) {
    scene_.set(camera_, "near", scene::Value::number(request.hither));
    scene_.set(camera_, "far", scene::Value::number(request.yon));
}

void SceneClient::depthOfField(const v3d::render::offline::rib::requests::DepthOfField & request) {
    scene_.set(camera_, "fstop", scene::Value::number(request.fstop));
    scene_.set(camera_, "focalLength", scene::Value::number(request.focalLength));
    scene_.set(camera_, "focalDistance", scene::Value::number(request.focalDistance));
}

void SceneClient::shutter(const v3d::render::offline::rib::requests::Shutter & request) {
    scene_.set(camera_, "shutter", scene::Value::floats({ request.open, request.close }));
}

void SceneClient::pixelSamples(const glm::uvec2 & samples) {
    scene_.set(options_, "samples", scene::Value::integer(static_cast<std::int32_t>(samples.x * samples.y)));
}

void SceneClient::pixelFilter(v3d::render::offline::film::Filter filter, float xwidth, float ywidth) {
    scene_.set(options_, "filter", scene::Value::string(v3d::render::offline::film::filterName(filter)));
    scene_.set(options_, "filterWidth", scene::Value::floats({ xwidth, ywidth }));
}

void SceneClient::pixelVariance(float variation) {
    scene_.set(options_, "variance", scene::Value::number(variation));
}

scene::Handle SceneClient::output(const std::string & file) {
    const auto found = outputs_.find(file);
    if (found != outputs_.end()) {
        return found->second;
    }
    const scene::Handle made = scene_.create("output", file);
    scene_.set(made, "file", scene::Value::string(file));
    outputs_[file] = made;
    return made;
}

void SceneClient::display(const v3d::render::offline::rib::requests::Display & request, const v3d::render::offline::rib::ParameterList & parameters) {
    (void)parameters;
    const bool adds = !request.name.empty() && request.name.front() == '+';
    const std::string file = adds ? request.name.substr(1) : request.name;
    if (!adds) {
        for (const auto & [name, made] : outputs_) {
            if (name != file) {
                scene_.remove(made);
            }
        }
        std::erase_if(outputs_, [&file](const auto & named) { return named.first != file; });
    }

    const scene::Handle made = output(file);
    std::string format = endsWith(file, ".png") ? "png" : "exr";
    if (request.type == "framebuffer") {
        format = "framebuffer";
    }
    scene_.set(made, "format", scene::Value::string(format));

    std::vector<scene::Handle> channels;
    if (request.mode != "rgb" && request.mode != "rgba" && !request.mode.empty()) {
        const auto held = channels_.find(request.mode);
        if (held == channels_.end()) {
            const scene::Handle channel = scene_.create("channel", request.mode);
            scene_.set(channel, "source", scene::Value::string(request.mode));
            channels_[request.mode] = channel;
        }
        channels.push_back(channels_[request.mode]);
    }
    scene_.set(made, "channels", scene::Value::handles(channels));
}

void SceneClient::frameBegin(int frame) {
    scene_.set(options_, "frame", scene::Value::integer(frame));
}

void SceneClient::worldBegin() {
    // the transformation now takes world space to camera space, and the camera holds the other way
    internal::Sampled camera = current_.transformation;
    for (glm::mat4 & matrix : camera.matrices) {
        matrix = glm::inverse(matrix);
    }
    camera.set(&scene_, camera_, "transform", logger_);
    current_.transformation = internal::Sampled();

    // a new world replaces everything the world before it made
    made_.clear();
    lights_.clear();
    objects_.clear();
    spaces_.clear();
    spaces_["camera"] = camera;
    current_.lights.clear();
    current_.material = boost::make_shared<scene::Handle>();
    world_ = true;
}

void SceneClient::worldEnd() {
    lights_.finish();
    world_ = false;
    scene_.commit();
    if (committed_) {
        committed_();
    }
}

void SceneClient::attributeBegin() {
    attributes_.push_back(current_);
}

void SceneClient::attributeEnd() {
    if (attributes_.empty()) {
        return;
    }
    current_ = attributes_.back();
    attributes_.pop_back();
}

void SceneClient::transformBegin() {
    transformations_.push_back(current_.transformation);
}

void SceneClient::transformEnd() {
    if (transformations_.empty()) {
        return;
    }
    current_.transformation = transformations_.back();
    transformations_.pop_back();
}

void SceneClient::concat(const glm::mat4 & matrix) {
    if (motion_) {
        // each request builds on the transformation the block opened with, at its own time
        const std::size_t at = poses_.size();
        const bool same = base_.times == times_;
        poses_.push_back((same && at < base_.matrices.size() ? base_.matrices[at] : base_.matrices.front()) * matrix);
        return;
    }
    for (glm::mat4 & sample : current_.transformation.matrices) {
        sample = sample * matrix;
    }
}

void SceneClient::replace(const glm::mat4 & matrix) {
    if (motion_) {
        poses_.push_back(matrix);
        return;
    }
    current_.transformation = internal::Sampled { .times = {}, .matrices = { matrix } };
}

void SceneClient::identity() {
    replace(glm::mat4(1.0F));
}

void SceneClient::transform(const glm::mat4x4 & matrix) {  // NOLINT(build/include_what_you_use)
    replace(matrix);
}

void SceneClient::concatTransform(const glm::mat4x4 & matrix) {
    concat(matrix);
}

void SceneClient::translate(float dx, float dy, float dz) {
    concat(glm::translate(glm::mat4(1.0F), glm::vec3(dx, dy, dz)));
}

// RiRotate takes its angle in degrees and glm takes radians
void SceneClient::rotate(float angle, const glm::vec3 & axis) {
    concat(glm::rotate(glm::mat4(1.0F), glm::radians(angle), axis));
}

void SceneClient::scale(float sx, float sy, float sz) {
    concat(glm::scale(glm::mat4(1.0F), glm::vec3(sx, sy, sz)));
}

void SceneClient::motionBegin(const std::vector<float> & times) {
    motion_ = true;
    times_ = times;
    base_ = current_.transformation;
    poses_.clear();
    deformations_.clear();
}

void SceneClient::motionEnd() {
    if (!motion_) {
        return;
    }
    motion_ = false;

    // a deforming primitive's poses become samples of its prototype, one at each of the block's
    // times
    deformations_.end(times_);

    // a block that held one request, or named one time, moves nothing
    const std::size_t count = std::min(poses_.size(), times_.size());
    if (count == 1 || (count == 0 && !poses_.empty())) {
        current_.transformation = internal::Sampled { .times = {}, .matrices = { poses_.front() } };
    } else if (count > 1) {
        current_.transformation = internal::Sampled { .times = std::vector<float>(times_.begin(), times_.begin() + static_cast<std::ptrdiff_t>(count)),
            .matrices = std::vector<glm::mat4>(poses_.begin(), poses_.begin() + static_cast<std::ptrdiff_t>(count)) };
    }
}

void SceneClient::attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name == "identifier" && parameters.has("name")) {
        current_.name = parameters.string("name");
    }
}

std::string SceneClient::nameFor(const std::string & kind, const std::string & request) {
    return made_.name(kind, request, current_.name);
}

void SceneClient::place(const internal::Shape & shape) {
    if (!world_) {
        warnings_.once("outside", "a primitive outside WorldBegin and WorldEnd is not drawn");
        return;
    }
    const scene::Handle prototype = made_.make(shape.kind, nameFor(shape.kind, shape.request));
    for (const auto & [name, value] : shape.parameters) {
        scene_.set(prototype, name, value);
    }

    // the request's other parameters are primvars, unless the kind holds one of the name
    std::set<std::string> except = shape.taken;
    const scene::Kind & kind = scene_.schema().kind(prototype.kind);
    for (const scene::Declaration & declared : kind.parameters) {
        except.insert(declared.name);
    }
    internal::declare(&scene_, prototype, shape.primvars, except, false);

    // the first pose of a deforming primitive, whose later poses add samples to it
    if (motion_) {
        deformations_.begin(prototype, shape);
    }

    const bool inObject = current_.group.valid();
    instance(prototype, nameFor("instance", shape.request), !inObject);

    if (current_.area && !inObject) {
        lights_.glow(prototype, current_.area->name, current_.area->parameters, current_.transformation);
    }
}

scene::Handle SceneClient::instance(const scene::Handle & prototype, const std::string & name, bool lit) {
    const scene::Handle placed = made_.make("instance", name);
    scene_.set(placed, "prototype", scene::Value::handle(prototype));
    current_.transformation.set(&scene_, placed, "transform", logger_);
    if (current_.group.valid()) {
        // an object's geometry takes the material in force where the object is placed
        scene_.set(placed, "group", scene::Value::handle(current_.group));
    } else {
        scene_.set(placed, "material", scene::Value::handle(material()));
    }
    if (lit) {
        lights_.lit(placed, current_.lights);
    }
    return placed;
}

scene::Handle SceneClient::shaderNode(const Shader & shader, const std::string & name) {
    const scene::Handle node = made_.make("shader", name);
    scene_.set(node, "shader", scene::Value::string(shader.name));
    scene_.set(node, "language", scene::Value::string("ri"));
    internal::declare(&scene_, node, shader.parameters, { "shader", "language" }, true);
    return node;
}

scene::Handle SceneClient::material() {
    if (current_.material->valid()) {
        return *current_.material;
    }
    // a material and its nodes are named after the surface shader, whatever geometry made them
    const scene::Handle surface = shaderNode(current_.surface, made_.unique("shader", current_.surface.name));
    for (const auto & [input, value] : { std::pair { "Cs", current_.colour }, std::pair { "Os", current_.opacity } }) {
        if (!current_.surface.parameters.has(input)) {
            scene_.declare(surface, scene::Declaration { .name = input, .type = scene::Type::Colour, .fallback = scene::Value::colour(value) });
        }
    }
    const scene::Handle made = made_.make("material", made_.unique("material", current_.surface.name));
    scene_.set(made, "surface", scene::Value::handle(surface));
    if (!current_.displacement.name.empty()) {
        const scene::Handle displacement = shaderNode(current_.displacement, made_.unique("shader", current_.displacement.name));
        scene_.set(made, "displacement", scene::Value::handle(displacement));
    }
    *current_.material = made;
    return made;
}

void SceneClient::coordinateSystem(const std::string & space) {
    if (!space.empty()) {
        spaces_[space] = current_.transformation;
    }
}

void SceneClient::coordSysTransform(const std::string & space) {
    if (space == "world") {
        current_.transformation = internal::Sampled();
        return;
    }
    const auto found = spaces_.find(space);
    if (found == spaces_.end()) {
        warnings_.once("space " + space, "the space \"" + space + "\" was not named, so CoordSysTransform changes nothing");
        return;
    }
    current_.transformation = found->second;
}

void SceneClient::color(const glm::vec3 & value) {
    current_.colour = value;
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::opacity(const glm::vec3 & value) {
    current_.opacity = value;
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::basis(const v3d::render::offline::rib::requests::Basis & request) {
    std::string name = v3d::render::offline::rib::basisName(request.v);
    if (name.empty()) {
        warnings_.once("basis", "moya's curves take a basis RI names, so a basis of its own numbers is taken as bezier");
        name = "bezier";
    }
    current_.basis = name;
}

void SceneClient::surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    current_.surface = Shader { .name = name, .parameters = parameters };
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::displacement(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name.empty()) {
        return;
    }
    current_.displacement = Shader { .name = name, .parameters = parameters };
    current_.material = boost::make_shared<scene::Handle>();
}

void SceneClient::lightSource(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) {
    if (request.shader.empty()) {
        return;
    }
    if (!world_) {
        warnings_.once("light outside", "a light outside WorldBegin and WorldEnd makes no light");
        return;
    }
    const std::string name = current_.name.empty() ? "light " + request.handle : current_.name;
    if (lights_.source(request, parameters, name, current_.transformation)) {
        current_.lights.insert(request.handle);
    }
}

void SceneClient::areaLightSource(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) {
    if (request.shader.empty()) {
        current_.area.reset();
        return;
    }
    current_.area = Shader { .name = current_.name.empty() ? "light " + request.handle : current_.name, .parameters = parameters };
}

void SceneClient::illuminate(const std::string & handle, bool on) {
    if (!lights_.named(handle)) {
        return;
    }
    if (on) {
        current_.lights.insert(handle);
    } else {
        current_.lights.erase(handle);
    }
}

v3d::render::offline::rib::Handler* SceneClient::deformation() {
    return &deformations_;
}

void SceneClient::polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::polygon(vertices, parameters));
}

void SceneClient::pointsPolygons(const v3d::render::offline::rib::requests::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::mesh(request, parameters));
}

void SceneClient::sphere(const v3d::render::offline::rib::requests::Sphere & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::sphere(request, parameters));
}

void SceneClient::curves(const v3d::render::offline::rib::requests::Curves & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::curves(request, parameters, current_.basis));
}

void SceneClient::points(unsigned int count, const v3d::render::offline::rib::ParameterList & parameters) {
    (void)count;
    place(internal::Shapes::points(parameters));
}

void SceneClient::cone(const v3d::render::offline::rib::requests::Cone & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::cone(request, parameters));
}

void SceneClient::cylinder(const v3d::render::offline::rib::requests::Cylinder & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::cylinder(request, parameters));
}

void SceneClient::hyperboloid(const v3d::render::offline::rib::requests::Hyperboloid & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::hyperboloid(request, parameters));
}

void SceneClient::paraboloid(const v3d::render::offline::rib::requests::Paraboloid & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::paraboloid(request, parameters));
}

void SceneClient::disk(const v3d::render::offline::rib::requests::Disk & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::disk(request, parameters));
}

void SceneClient::torus(const v3d::render::offline::rib::requests::Torus & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::torus(request, parameters));
}

void SceneClient::subdivisionMesh(const v3d::render::offline::rib::requests::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters) {
    place(internal::Shapes::subdivision(request, parameters, &warnings_));
}

void SceneClient::objectBegin(const std::string & handle) {
    if (!world_) {
        warnings_.once("object outside", "an object outside WorldBegin and WorldEnd is not kept");
    }
    attributes_.push_back(current_);
    const scene::Handle group = made_.make("group", made_.unique("group", current_.name.empty() ? "object " + handle : current_.name));
    objects_[handle] = group;
    current_.transformation = internal::Sampled();
    current_.group = group;
}

void SceneClient::objectEnd() {
    if (attributes_.empty() || !current_.group.valid()) {
        return;
    }
    current_ = attributes_.back();
    attributes_.pop_back();
}

void SceneClient::objectInstance(const std::string & handle) {
    const auto found = objects_.find(handle);
    if (found == objects_.end()) {
        warnings_.once("object " + handle, "ObjectInstance names the object " + handle + ", which no ObjectBegin made");
        return;
    }
    instance(found->second, nameFor("instance", "object " + handle), !current_.group.valid());
}

};  // namespace v3d::moya::ri
