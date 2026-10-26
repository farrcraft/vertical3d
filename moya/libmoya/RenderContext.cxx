/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderContext.h"

#include <api/log/Logger.h>
#include <api/render/offline/Area.h>
#include <api/render/offline/Camera.h>
#include <api/render/offline/CameraOptions.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/Sampling.h>
#include <api/render/offline/Textures.h>
#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Requests.h>
#include <api/render/offline/sl/Imager.h>
#include <api/render/offline/sl/Instance.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/sl/Types.h>
#include <api/render/offline/trace/Primitive.h>
#include <api/render/offline/trace/Scene.h>
#include <moya/libmoya/reyes/GridShader.h>
#include <moya/libmoya/reyes/Hider.h>

#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <limits>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include "Display.h"
#include "Planes.h"
#include "Hider.h"
#include "Lights.h"
#include "Polygon.h"
#include "Progress.h"
#include "RayHider.h"
#include "Shaders.h"
#include "Shading.h"
#include "TracedScene.h"

namespace v3d::moya {

glm::vec3 project(const glm::mat4x4 & matrix, const glm::vec3 & point) {
    return v3d::render::offline::sl::project(matrix, point);
}

RenderContext::RenderContext() {
    initialize();
}

RenderContext::RenderContext(const std::string& name) : name_(name) {
    initialize();
}

RenderContext::~RenderContext() {
}

void RenderContext::initialize() {
    logger_ = boost::make_shared<v3d::log::Logger>();
    // RI's default hider
    hider_ = boost::make_shared<reyes::Hider>();
    shaders_ = boost::make_shared<Shaders>(logger_);
}

boost::shared_ptr<v3d::render::offline::FrameBuffer> RenderContext::picture() const {
    return planes_;
}

std::size_t RenderContext::bucketed() const {
    return hider_->bucketed();
}

unsigned int RenderContext::bucketWidth() const {
    return bucketWidth_;
}

unsigned int RenderContext::bucketHeight() const {
    return bucketHeight_;
}

unsigned int RenderContext::gridSize() const {
    return gridSize_;
}

float RenderContext::shadingRate() const {
    return attributes_.shadingRate;
}

v3d::render::offline::Sampling & RenderContext::sampling() {
    return sampling_;
}

const v3d::render::offline::Sampling & RenderContext::sampling() const {
    return sampling_;
}

unsigned int RenderContext::samplesTaken(const glm::uvec2 & pixel) const {
    return hider_->samplesTaken(pixel);
}

void RenderContext::prepareWorld() {
    // a scene that named no projection gets the default
    if (!options_.projectionNamed()) {
        projection("orthographic");
    }

    // what a scene set between RiProjection and here is the world to camera transformation.
    // The camera is built from the options as they stand now, because RI lets a screen window,
    // format or clipping follow the projection
    camera_ = options_.freeze(transforms_.current().reference());
    traced_.scene().camera(camera_);
    // inside the world block the current transformation is object to world
    transforms_.current().replace(glm::mat4x4(1.0F));

    planes_ = Planes::make(options_.width(), options_.height());
    hider_->begin(this);
}

unsigned int RenderContext::imageWidth() const {
    return options_.width();
}

unsigned int RenderContext::imageHeight() const {
    return options_.height();
}

float RenderContext::pixelAspect() const {
    return options_.pixelAspect();
}

bool RenderContext::perspective() const {
    return options_.perspective();
}

const v3d::render::offline::Camera & RenderContext::camera() const {
    return camera_;
}

void RenderContext::projection(std::string name, float fov) {
    // the projection applies after the transformation in force here
    if (!options_.projection(name, fov, transforms_.current().reference())) {
        logger_->get()->warn("the projection '{}' is not known, and is not used", name);
        return;
    }
    camera_.screen = options_.screen();

    // what the scene sets from here to WorldBegin is the world to camera transformation
    transforms_.current().replace(glm::mat4x4(1.0F));
}

void RenderContext::hider(const std::string & name) {
    if (name == "hidden") {
        hider_ = boost::make_shared<reyes::Hider>();
    } else if (name == "raytrace") {
        hider_ = boost::make_shared<RayHider>();
    } else {
        logger_->get()->warn("moya has no hider named '{}', so it keeps the one it had", name);
    }
}

bool RenderContext::raytracing() const {
    return hider_->traces();
}

float RenderContext::hither() const {
    return options_.hither();
}

bool RenderContext::imageResolution(const glm::ivec2 & resolution, float aspect) {
    return options_.format(resolution, aspect);
}

void RenderContext::frameAspectRatio(float aspect) {
    options_.frameAspectRatio(aspect);
}

void RenderContext::screenWindow(const v3d::render::offline::rib::ScreenWindow & window) {
    options_.screenWindow(window);
}

void RenderContext::clipping(const v3d::render::offline::rib::Clipping & planes) {
    options_.clipping(planes);
}

void RenderContext::pushTransform(void) {
    transforms_.push();
}

void RenderContext::popTransform(void) {
    transforms_.pop();
}

void RenderContext::attributeBegin() {
    saved_.push_back({.transform = transforms_.current(), .attributes = attributes_});
}

void RenderContext::attributeEnd() {
    if (saved_.empty()) {
        return;
    }
    // the lights a block created last for the frame. Only the set that is on is an attribute
    transforms_.current() = saved_.back().transform;
    attributes_ = saved_.back().attributes;
    saved_.pop_back();
}

void RenderContext::saveCoordinateSystem(const std::string& name) {
    if (!transforms_.save(name)) {
        logger_->get()->warn("'{}' is a predefined coordinate system, and a scene cannot name one", name);
        return;
    }
    // a shader reads the spaces the scene named, under either hider, through the traced scene
    traced_.scene().named(transforms_.named());
}

bool RenderContext::known(const std::string& name) const {
    return transforms_.known(name);
}

void RenderContext::setCoordinateSystem(const std::string& name) {
    if (!known(name)) {
        logger_->get()->warn("the coordinate system '{}' is not known, and the transformation is kept", name);
        return;
    }
    transforms_.current().replace(coordinateSystem(name));
}

void RenderContext::setIdentityTransform() {
    transforms_.current().replace(glm::mat4(1.0F));
}

void RenderContext::setTransform(const glm::mat4x4& trans) {
    transforms_.current().replace(trans);
}

void RenderContext::motionBegin(const std::vector<float> & times) {
    transforms_.current().begin(times);
}

void RenderContext::motionEnd() {
    transforms_.current().end();
}

void RenderContext::concatTransform(const glm::mat4x4& trans) {
    transforms_.current().concat(trans);
}

void RenderContext::color(const glm::vec3& value) {
    attributes_.color = value;
}

glm::vec3 RenderContext::color() const {
    return attributes_.color;
}

void RenderContext::opacity(const glm::vec3& value) {
    attributes_.opacity = value;
}

glm::vec3 RenderContext::opacity() const {
    return attributes_.opacity;
}

void RenderContext::shadingRate(float size) {
    attributes_.shadingRate = size;
}

void RenderContext::bucketSize(unsigned int width, unsigned int height) {
    if (bucketPinned_) {
        return;
    }
    // a bucket of no pixels covers nothing and the frame would never be finished
    if (width == 0 || height == 0) {
        logger_->get()->warn("a bucket of {} by {} pixels is not used", width, height);
        return;
    }
    bucketWidth_ = width;
    bucketHeight_ = height;
}

void RenderContext::gridSize(unsigned int size) {
    if (gridPinned_) {
        return;
    }
    // a grid of no micropolygons would split every primitive without end
    if (size == 0) {
        logger_->get()->warn("a grid size of 0 is not used");
        return;
    }
    gridSize_ = size;
}

void RenderContext::pinBucketSize(unsigned int width, unsigned int height) {
    bucketPinned_ = false;
    bucketSize(width, height);
    bucketPinned_ = width != 0 && height != 0;
}

void RenderContext::pinGridSize(unsigned int size) {
    gridPinned_ = false;
    gridSize(size);
    gridPinned_ = size != 0;
}

void RenderContext::motionCache(std::size_t entries) {
    motionCache_ = entries;
}

std::size_t RenderContext::motionCache() const {
    return motionCache_;
}

glm::mat4x4 RenderContext::coordinateSystem(const std::string& name) {
    glm::mat4x4 matrix(1.0F);
    if (!transforms_.space(name, camera_, &matrix)) {
        logger_->get()->warn("the coordinate system '{}' is not known, and the identity is used", name);
    }
    return matrix;
}

void RenderContext::translate(float dx, float dy, float dz) {
    transforms_.current().concat(glm::translate(glm::mat4x4(1.0F), glm::vec3(dx, dy, dz)));
}

// RiRotate takes its angle in degrees and glm takes radians
void RenderContext::rotate(float angle, const glm::vec3 & axis) {
    transforms_.current().concat(glm::rotate(glm::mat4x4(1.0F), glm::radians(angle), axis));
}

void RenderContext::scale(float sx, float sy, float sz) {
    transforms_.current().concat(glm::scale(glm::mat4x4(1.0F), glm::vec3(sx, sy, sz)));
}


void RenderContext::surface(const std::string & name,
    const v3d::render::offline::rib::ParameterList & parameters) {
    attributes_.surface.shader = shaders_->instance(name, v3d::render::offline::sl::ShaderType::SURFACE, parameters);
    // RI makes a shader's own space the transformation in force when the scene instanced
    // it. A "point \"shader\" (0, 0, 1)" parameter is stated in that space
    attributes_.surface.placement = coordinateSystem("camera") * transforms_.current().reference();
}

void RenderContext::displacement(const std::string & name,
    const v3d::render::offline::rib::ParameterList & parameters) {
    attributes_.displacement.shader = shaders_->instance(name, v3d::render::offline::sl::ShaderType::DISPLACEMENT, parameters);
    attributes_.displacement.placement = coordinateSystem("camera") * transforms_.current().reference();
}

void RenderContext::lightSource(const v3d::render::offline::rib::LightSource & request,
    const v3d::render::offline::rib::ParameterList & parameters) {
    Lights::Light light;
    light.handle = request.handle;
    light.shader = shaders_->instance(request.shader, v3d::render::offline::sl::ShaderType::LIGHT, parameters);
    light.placement = coordinateSystem("camera") * transforms_.current().reference();
    if (!light.shader) {
        // the library has already logged why. The light is left out, not replaced by a
        // light of some other kind
        return;
    }
    lights_.add(light);
    // RiLightSource switches the light it creates on
    illuminate(request.handle, true);
}

void RenderContext::areaLightSource(const v3d::render::offline::rib::LightSource & request,
    const v3d::render::offline::rib::ParameterList & parameters) {
    const std::size_t before = lights_.size();
    lightSource(request, parameters);
    if (lights_.size() == before) {
        return;
    }
    attributes_.areaLight = lights_.area(attributes_.lightSamples);
}

void RenderContext::attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) {
    if (name == "light" && parameters.has("nsamples")) {
        const float count = parameters.number("nsamples", 1.0F);
        // capped, so a count past what an unsigned holds is not cast into one
        constexpr float MOST_SAMPLES = 4096.0F;
        attributes_.lightSamples = std::isfinite(count) && count >= 1.0F ? static_cast<unsigned int>(std::min(count, MOST_SAMPLES)) : 1U;
        return;
    }
    if (name == "displacementbound" && parameters.has("sphere")) {
        const float radius = parameters.number("sphere", 0.0F);
        if (!std::isfinite(radius) || radius < 0.0F) {
            logger_->get()->warn("a displacement bound of {} is not a distance, and is not used", radius);
            return;
        }
        // the bound is stated in object space, or the space it names, and is kept in camera
        // space, where every primitive is culled. A scale that differs between axes is taken
        // at its largest, so the bound still holds the surface
        const std::vector<std::string> & named = parameters.strings("coordinatesystem");
        glm::mat4x4 toCamera = coordinateSystem("camera") * transforms_.current().reference();
        if (!named.empty() && named.front() == "camera") {
            toCamera = glm::mat4x4(1.0F);
        } else if (!named.empty() && named.front() != "object" && named.front() != "world" &&
            !known(named.front())) {
            logger_->get()->warn("the displacement bound's space \"{}\" is not one the scene named, so object space is used",
                named.front());
        } else if (!named.empty() && named.front() == "world") {
            toCamera = coordinateSystem("camera");
        } else if (!named.empty() && named.front() != "object") {
            // a named space is saved as the transformation into world space in force when the
            // scene named it
            toCamera = coordinateSystem("camera") * coordinateSystem(named.front());
        }
        const glm::mat3 linear(toCamera);
        const float stretch = std::max(glm::length(linear[0]), std::max(glm::length(linear[1]), glm::length(linear[2])));
        attributes_.displacementBound = radius * stretch;
        return;
    }
    if (std::find(unusedAttributes_.begin(), unusedAttributes_.end(), name) == unusedAttributes_.end()) {
        unusedAttributes_.push_back(name);
        logger_->get()->warn("the attribute \"{}\" is not used", name);
    }
}

const boost::shared_ptr<v3d::render::offline::Area> & RenderContext::areaLight() const {
    return attributes_.areaLight;
}

void RenderContext::illuminate(const std::string & handle, bool on) {
    attributes_.illuminate(handle, on);
}

void RenderContext::imager(const std::string & name,
    const v3d::render::offline::rib::ParameterList & parameters) {
    shaders_->imager(name, parameters);
}

void RenderContext::searchpath(const std::string & path) {
    shaders_->searchpath(path);
}

v3d::render::offline::Textures & RenderContext::textures() {
    return shaders_->textures();
}

Shading RenderContext::shading() {
    Shading state;
    state.surface = attributes_.surface.shader;
    state.placement = attributes_.surface.placement;
    state.object = coordinateSystem("camera") * transforms_.current().reference();
    state.opacity = attributes_.opacity;
    if (!state.surface) {
        // a scene that names no surface draws "constant", which does no shading. RI leaves
        // the default surface to the renderer
        state.surface = shaders_->constant();
        state.placement = coordinateSystem("camera") * transforms_.current().reference();
    }
    state.lights = lights_.shining(attributes_.lit);
    state.displacement = attributes_.displacement;
    state.displacementBound = attributes_.displacementBound;
    return state;
}

v3d::render::offline::rib::Declarations & RenderContext::declarations() {
    return declarations_;
}

const boost::shared_ptr<v3d::log::Logger> & RenderContext::logger() const {
    return logger_;
}

reyes::GridShader & RenderContext::shader() {
    if (!shader_) {
        shader_ = boost::make_shared<reyes::GridShader>(this);
    }
    return *shader_;
}

v3d::render::offline::trace::Scene & RenderContext::traced() {
    return traced_.scene();
}

TracedScene::Submission RenderContext::submission(const Shading & state) {
    TracedScene::Submission with;
    with.transform = &transforms_.current();
    with.colour = attributes_.color;
    with.area = attributes_.areaLight.get();
    with.view = coordinateSystem("camera");
    // a light's placement takes it to camera space, and the traced scene is in world space
    with.lights = lights_.traced(state.lights, attributes_.lit, glm::inverse(with.view));
    with.displacer = &shader();
    return with;
}

bool RenderContext::addSphere(const v3d::render::offline::rib::Sphere & shape) {
    // an area light keeps its sphere whichever hider draws it, and the reyes hider draws none
    if (attributes_.areaLight && shape.radius > 0.0F) {
        attributes_.areaLight->add({.placement = transforms_.current().reference(), .radius = shape.radius});
    }
    if (!hider_->traces()) {
        return false;
    }
    if (!(shape.radius > 0.0F)) {
        logger_->get()->warn("a sphere of radius {} is not drawn", shape.radius);
        return true;
    }
    if (flatMotion()) {
        return true;
    }
    const Shading state = shading();
    traced_.add(shape, state, submission(state));
    return true;
}

bool RenderContext::flatMotion() {
    if (!transforms_.current().moving() || transforms_.current().placeable()) {
        return false;
    }
    // both ends of its motion flatten it, so there is no pose to move it from
    if (!flatMotionReported_) {
        flatMotionReported_ = true;
        logger_->get()->warn("a primitive whose motion is flat at both ends is not drawn");
    }
    return true;
}

bool RenderContext::beforeWorld() {
    if (planes_) {
        return false;
    }
    if (!beforeWorldReported_) {
        beforeWorldReported_ = true;
        logger_->get()->warn("a polygon sent before WorldBegin is not drawn");
    }
    return true;
}

void RenderContext::addPolygon(const boost::shared_ptr<Polygon>& poly) {
    if (beforeWorld() || flatMotion()) {
        return;
    }
    if (hider_->traces()) {
        // the ray hider sees the traced scene and nothing else, so nothing is bucketed
        const Shading state = shading();
        traced_.add(*poly, state, submission(state));
        return;
    }
    // a polygon is placed with the state it was submitted under, and is traced once. A piece of
    // a split is already placed, keeps its parent's state, and goes back to the hider itself
    if (!poly->placed()) {
        const Shading state = shading();
        traced_.add(*poly, state, submission(state));
        poly->place(coordinateSystem("camera") * transforms_.current().reference(), attributes_.color,
            poly->geometricNormal(), state);
        poly->motion(transforms_.current().before(coordinateSystem("camera")));
    }
    hider_->add(poly, this);
}

void RenderContext::display(const v3d::render::offline::rib::Display & display) {
    display_.request(display);
}

void RenderContext::display(const v3d::render::offline::rib::Display & display,
    const v3d::render::offline::rib::ParameterList & parameters) {
    this->display(display);
    for (const std::string & name : parameters.names()) {
        logger_->get()->warn("the display parameter '{}' is not used", name);
    }
}

void RenderContext::cropWindow(const v3d::render::offline::rib::CropWindow & window) {
    if (!display_.crop(window)) {
        logger_->get()->warn("a crop window from ({}, {}) to ({}, {}) has no area, and is not used",
            window.xmin, window.ymin, window.xmax, window.ymax);
    }
}

glm::uvec4 RenderContext::cropped() const {
    return display_.cropped(options_.width(), options_.height());
}

const std::string & RenderContext::displayName() const {
    return display_.name();
}

void RenderContext::progress(const Progress::Callback& callback) {
    progress_.callback(callback);
}

bool RenderContext::report(float done) {
    return progress_.report(done);
}

bool RenderContext::stopped() const noexcept {
    return progress_.stopped();
}

void RenderContext::render() {
    if (!planes_) {
        return;
    }

    progress_.begin();
    // the reyes hider meets the traced scene only through a shader's trace or transmission
    if (!hider_->traces() && !shaders_->traces()) {
        traced_.scene().clear();
    }
    const boost::shared_ptr<v3d::render::offline::FrameBuffer> & planes = planes_;
    // the hider's time is logged, so that a render can be timed again after a change
    const std::chrono::steady_clock::time_point started = std::chrono::steady_clock::now();
    hider_->render(this, planes.get());
    const std::chrono::duration<double> took = std::chrono::steady_clock::now() - started;
    logger_->get()->info("rendered {} by {} pixels in {:.3f} s", planes->width(), planes->height(), took.count());
    if (progress_.stopped()) {
        return;
    }

    if (shaders_->imager()) {
        // the imager runs once the hider has finished, because it reads the whole picture.
        // The shader's machine logs a failed run. The failed row and the rows below it stay
        // as the hider wrote them, and the frame is still written
        v3d::render::offline::sl::Imager imager(shaders_->imager(), &shader());
        imager.run(planes_.get(), Planes::COVERAGE);
    }

    display_.finish(planes_.get(), logger_);
}

};  // namespace v3d::moya
