/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderContext.h"

#include <api/image/Factory.h>
#include <api/render/offline/sl/Imager.h>
#include <api/render/offline/trace/Scene.h>
#include <api/render/offline/trace/Sphere.h>
#include <api/render/offline/trace/Triangle.h>
#include <api/type/geometry/Frustum.h>
#include <moya/libmoya/reyes/GridShader.h>
#include <moya/libmoya/reyes/Hider.h>

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <limits>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include "Hider.h"
#include "RayHider.h"

namespace v3d::moya {

namespace {

/** The least w a point is divided by. A point at or below it is at or behind the eye. **/
constexpr float SMALLEST_W = 1.0e-6F;

};  // namespace

glm::vec3 project(const glm::mat4x4 & matrix, const glm::vec3 & point) {
    const glm::vec4 projected = matrix * glm::vec4(point, 1.0F);
    if (projected.w <= SMALLEST_W) {
        return glm::vec3(projected);
    }
    return glm::vec3(projected) / projected.w;
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
    shaders_ = boost::make_shared<v3d::render::offline::sl::ShaderLibrary>(logger_);
    textures_ = boost::make_shared<v3d::render::offline::Textures>(logger_);

    // the predefined coordinate systems start as the identity
    const glm::mat4x4 def(1.0F);
    coordinateSystems_["object"] = def;
    coordinateSystems_["world"] = def;
    coordinateSystems_["camera"] = def;
    coordinateSystems_["screen"] = def;
    coordinateSystems_["raster"] = def;
    coordinateSystems_["NDC"] = def;
}

boost::shared_ptr<FrameBuffer> RenderContext::framebuffer() const {
    return frameBuffer_;
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
    return shadingRate_;
}

reyes::Samples & RenderContext::samples() {
    assert(samples_);
    return *samples_;
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
    // a scene that named no projection gets the default. A named projection has already
    // reset the current transformation, so only its screen transform is built again, from
    // the screen window, format and clipping as they stand now
    if (!projectionNamed_) {
        projection("");
    } else {
        screenTransform();
    }

    // what a scene set between RiProjection and here is the world to camera transformation
    saveCoordinateSystem("camera");
    // the traced scene has no camera of its own, so a traced hit's "camera" space and E
    // are this camera's
    traced_.view(coordinateSystems_["camera"]);
    // inside the world block the current transformation is object to world
    transform_.replace(glm::mat4x4(1.0F));

    // set raster transformation
    glm::mat4x4 raster(1.0F);
    /*
        The screen transform maps to the canonical volume [-1, 1], and the raster transform
        takes that to [0, xres] by [0, yres].

        y is negated because raster space has its origin at the upper left corner and
        counts downward. image::Image stores its rows in the same order, top row first.
    */
    const float x = static_cast<float>(xres_) / 2.0F;
    const float y = static_cast<float>(yres_) / 2.0F;
    raster = glm::scale(raster, glm::vec3(x, -y, 1.0F));
    raster = glm::translate(raster, glm::vec3(1.0F, -1.0F, 1.0F));

    coordinateSystems_["raster"] = raster;

    frameBuffer_.reset(new FrameBuffer({.bucket = glm::uvec2(bucketWidth_, bucketHeight_), .image = glm::uvec2(xres_, yres_)}));
}

unsigned int RenderContext::imageWidth() const {
    return xres_;
}

unsigned int RenderContext::imageHeight() const {
    return yres_;
}

float RenderContext::pixelAspect() const {
    return pixelAspect_;
}

bool RenderContext::perspective() const {
    return projection_ == "perspective";
}

void RenderContext::projection(std::string name, float fov) {
    if (name.empty()) {
        name = "orthographic";
    }

    // only "perspective" reads fov
    projection_ = name;
    fov_ = fov;
    projectionNamed_ = true;

    // the projection applies after the transformation in force here. It is kept so that
    // WorldBegin can build the screen transform again, because RI lets a screen window,
    // format or clipping follow the projection
    projectionBase_ = transform_;
    screenTransform();

    // what the scene sets from here to WorldBegin is the world to camera transformation
    transform_.replace(glm::mat4x4(1.0F));
}

void RenderContext::screenTransform() {
    // append the projection to the transformation in force at RiProjection. RI states the
    // composition in row vectors, where the projection is on the right; a matrix applies to
    // what is on its right here, so it goes on the left
    const v3d::render::offline::MovingTransform current = transform_;
    transform_ = projectionBase_.before(projectionMatrix());
    // save as screen coordinate system
    saveCoordinateSystem("screen");
    transform_ = current;
}

glm::mat4x4 RenderContext::projectionMatrix() const {
    const float left = screen_[0];
    const float right = screen_[1];
    const float bottom = screen_[2];
    const float top = screen_[3];

    // the identity, because an unsupported projection takes none of the branches below
    glm::mat4x4 projection(1.0F);
    if (projection_ == "perspective") {
        /*
            RI states fov as the full angle between screen space (-1, 0) and (1, 0), so a
            point at eye depth z reaches screen x = 1 at x = z * tan(fov / 2). The screen
            window then selects the part of screen space the image covers.

            The interface looks down +z, so w is +z rather than the -z a right handed
            system would write. Depth runs [-1, 1] to match the orthographic branch, and
            the frustum cull in addPolygon() reads the same range.
        */
        const float tangent = std::tan(glm::radians(fov_) / 2.0F);
        projection = glm::mat4x4(0.0F);
        projection[0][0] = 2.0F / ((right - left) * tangent);
        projection[1][1] = 2.0F / ((top - bottom) * tangent);
        projection[2][0] = -(right + left) / (right - left);
        projection[2][1] = -(top + bottom) / (top - bottom);
        projection[2][2] = (far_ + near_) / (far_ - near_);
        projection[2][3] = 1.0F;
        projection[3][2] = -2.0F * far_ * near_ / (far_ - near_);
    } else if (projection_ == "orthographic") {
        /*
            The screen window maps to [-1, 1] on x and y, and [near, far] maps to [-1, 1]
            on z. glm indexes a matrix as [column][row], so the translation is column 3.

            [ 2 / (right - left)   0                    0                  -tx ]
            [ 0                    2 / (top - bottom)   0                  -ty ]
            [ 0                    0                    2 / (far - near)   -tz ]
            [ 0                    0                    0                    1 ]

            tx = (right + left) / (right - left)
            ty = (top + bottom) / (top - bottom)
            tz = (far + near) / (far - near)
        */

        const float far = far_;
        const float near = near_;

        const float tx = ((right + left) / (right - left));
        const float ty = ((top + bottom) / (top - bottom));
        const float tz = (far + near) / (far - near);

        projection[0][0] = 2.0F / (right - left);
        projection[1][0] = 0.0;
        projection[2][0] = 0.0;
        projection[3][0] = -tx;
        projection[0][1] = 0.0;
        projection[1][1] = 2.0F / (top - bottom);
        projection[2][1] = 0.0;
        projection[3][1] = -ty;
        projection[0][2] = 0.0;
        projection[1][2] = 0.0;
        // positive, because the interface looks down +z. The negated form is for a right
        // handed system, and here it would put the whole clip range behind the near plane
        projection[2][2] = 2.0F / (far - near);
        projection[3][2] = -tz;
        projection[0][3] = 0.0;
        projection[1][3] = 0.0;
        projection[2][3] = 0.0;
        projection[3][3] = 1.0;
    } else {  // an implementation-specific projection
        // moya has none, so an unsupported name projects through the identity
    }

    return projection;
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
    return near_;
}

bool RenderContext::imageResolution(const glm::ivec2 & resolution, float aspect) {
    // RI reads a side of zero or less as the device's default for that side
    const unsigned int width = resolution.x > 0 ? static_cast<unsigned int>(resolution.x) : defaultWidth;
    const unsigned int height = resolution.y > 0 ? static_cast<unsigned int>(resolution.y) : defaultHeight;
    if (width > v3d::render::offline::largestResolution || height > v3d::render::offline::largestResolution) {
        return false;
    }
    xres_ = width;
    yres_ = height;
    pixelAspect_ = aspect > 0.0F && std::isfinite(aspect) ? aspect : 1.0F;
    if (!frameAspectNamed_) {
        frameAspectRatio(static_cast<float>(xres_) * pixelAspect_ / static_cast<float>(yres_));
        frameAspectNamed_ = false;
    }
    return true;
}

void RenderContext::frameAspectRatio(float aspect) {
    frameAspect_ = aspect;
    frameAspectNamed_ = true;
    if (screenNamed_) {
        return;
    }
    // the RI default: the shorter side spans [-1, 1] and the longer spans [-a, a], so that
    // a frame which is not square does not stretch a square window across itself
    if (frameAspect_ >= 1.0F) {
        screenWindow({.left = -frameAspect_, .right = frameAspect_, .bottom = -1.0F, .top = 1.0F});
    } else {
        screenWindow({.left = -1.0F, .right = 1.0F, .bottom = -1.0F / frameAspect_, .top = 1.0F / frameAspect_});
    }
    screenNamed_ = false;
}

void RenderContext::screenWindow(const v3d::render::offline::rib::ScreenWindow & window) {
    screen_[0] = window.left;
    screen_[1] = window.right;
    screen_[2] = window.bottom;
    screen_[3] = window.top;
    screenNamed_ = true;
}

void RenderContext::clipping(const v3d::render::offline::rib::Clipping & planes) {
    near_ = planes.hither;
    far_ = planes.yon;
}

void RenderContext::pushTransform(void) {
    transforms_.push_back(transform_);
}

void RenderContext::popTransform(void) {
    if (transforms_.empty()) {
        return;
    }
    transform_ = transforms_.back();
    transforms_.pop_back();
}

void RenderContext::attributeBegin() {
    Attributes saved;
    saved.transform = transform_;
    saved.color = color_;
    saved.opacity = opacity_;
    saved.shadingRate = shadingRate_;
    saved.surface = surface_;
    saved.surfacePlacement = surfacePlacement_;
    saved.lit = lit_;
    attributes_.push_back(saved);
}

void RenderContext::attributeEnd() {
    if (attributes_.empty()) {
        return;
    }
    const Attributes & saved = attributes_.back();
    transform_ = saved.transform;
    color_ = saved.color;
    opacity_ = saved.opacity;
    shadingRate_ = saved.shadingRate;
    surface_ = saved.surface;
    surfacePlacement_ = saved.surfacePlacement;
    // only the set of lights that are on is restored. A light lasts for the frame, not
    // for the block that created it
    lit_ = saved.lit;
    attributes_.pop_back();
}

void RenderContext::saveCoordinateSystem(const std::string& name) {
    coordinateSystems_[name] = transform_.reference();
}

void RenderContext::setCoordinateSystem(const std::string& name) {
    transform_.replace(coordinateSystems_[name]);
}

void RenderContext::setIdentityTransform() {
    transform_.replace(glm::mat4(1.0F));
}

void RenderContext::setTransform(const glm::mat4x4& trans) {
    transform_.replace(trans);
}

void RenderContext::motionBegin(const std::vector<float> & times) {
    transform_.begin(times);
}

void RenderContext::motionEnd() {
    transform_.end();
}

void RenderContext::concatTransform(const glm::mat4x4& trans) {
    transform_.concat(trans);
}

void RenderContext::color(const glm::vec3& value) {
    color_ = value;
}

glm::vec3 RenderContext::color() const {
    return color_;
}

void RenderContext::opacity(const glm::vec3& value) {
    opacity_ = value;
}

glm::vec3 RenderContext::opacity() const {
    return opacity_;
}

void RenderContext::shadingRate(float size) {
    shadingRate_ = size;
}

void RenderContext::bucketSize(unsigned int width, unsigned int height) {
    // a bucket of no pixels covers nothing and the frame would never be finished
    if (width == 0 || height == 0) {
        logger_->get()->warn("a bucket of {} by {} pixels is not used", width, height);
        return;
    }
    bucketWidth_ = width;
    bucketHeight_ = height;
}

void RenderContext::gridSize(unsigned int size) {
    // a grid of no micropolygons would split every primitive without end
    if (size == 0) {
        logger_->get()->warn("a grid size of 0 is not used");
        return;
    }
    gridSize_ = size;
}

void RenderContext::motionCache(std::size_t entries) {
    motionCache_ = entries;
}

std::size_t RenderContext::motionCache() const {
    return motionCache_;
}

glm::mat4x4 RenderContext::coordinateSystem(const std::string& name) {
    return coordinateSystems_[name];
}

void RenderContext::translate(float dx, float dy, float dz) {
    transform_.concat(glm::translate(glm::mat4x4(1.0F), glm::vec3(dx, dy, dz)));
}

// RiRotate takes its angle in degrees and glm takes radians
void RenderContext::rotate(float angle, const glm::vec3 & axis) {
    transform_.concat(glm::rotate(glm::mat4x4(1.0F), glm::radians(angle), axis));
}

void RenderContext::scale(float sx, float sy, float sz) {
    transform_.concat(glm::scale(glm::mat4x4(1.0F), glm::vec3(sx, sy, sz)));
}

namespace {

/** An axis aligned box, by its least and greatest corners. **/
struct Bound {
    glm::vec3 min{0.0F};
    glm::vec3 max{0.0F};
};

/**
 * One of the eight corners of a box. Bit 0 of the index picks x, bit 1 picks y and bit 2
 * picks z, each from max when set and from min when clear.
 **/
glm::vec3 boxCorner(const glm::vec3 & min, const glm::vec3 & max, int index) {
    return glm::vec3((index & 1) != 0 ? max.x : min.x,
                     (index & 2) != 0 ? max.y : min.y,
                     (index & 4) != 0 ? max.z : min.z);
}

/**
 * The axis aligned bound of a box carried through an affine matrix.
 *
 * All eight corners go through the matrix. Under a rotation any corner can hold the extreme
 * on an axis, so the two diagonal corners alone can leave part of the box outside.
 **/
Bound transformBound(const glm::mat4x4 & transform, const Bound & object) {
    Bound bound{.min = glm::vec3(std::numeric_limits<float>::max()), .max = glm::vec3(std::numeric_limits<float>::lowest())};
    for (int k = 0; k < 8; k++) {
        const glm::vec3 corner(transform * glm::vec4(boxCorner(object.min, object.max, k), 1.0F));
        bound.min = glm::min(bound.min, corner);
        bound.max = glm::max(bound.max, corner);
    }
    return bound;
}

/**
 * Grow an eye space bound to cover where a moving primitive is at both ends of the shutter,
 * from its object space bound. A still primitive's bound is left as it is.
 **/
void sweep(const v3d::render::offline::MovingTransform & motion, const Bound & object, Bound* swept) {
    if (!motion.moving()) {
        return;
    }
    for (const glm::mat4x4* end : { &motion.open(), &motion.close() }) {
        const Bound at = transformBound(*end, object);
        swept->min = glm::min(swept->min, at.min);
        swept->max = glm::max(swept->max, at.max);
    }
}

/**
 * Give every vertex that brought no colour or normal of its own the primitive's, and every
 * vertex the primitive's plane as its geometric normal.
 *
 * A vertex with no "Cs" takes the primitive's colour, which the surface shader then reads as
 * Cs. The normals go the same way: Ng is the primitive's plane on every vertex, and a vertex
 * that brought no varying "N" shades with it, so a polygon that gives no normals is shaded
 * faceted.
 **/
void fillVertices(Polygon* poly) {
    for (unsigned int i = 0; i < poly->vertexCount(); i++) {
        if (!(*poly)[i].hasColor()) {
            (*poly)[i].color(poly->color());
        }
        (*poly)[i].geometricNormal(poly->normal());
        if (!(*poly)[i].hasNormal()) {
            (*poly)[i].normal(poly->normal());
        }
    }
}

};  // namespace

void RenderContext::surface(const std::string & name,
    const v3d::render::offline::rib::ParameterList & parameters) {
    surface_ = shaders_->instance(name, v3d::render::offline::sl::ShaderType::SURFACE, parameters);
    // RI makes a shader's own space the transformation in force when the scene instanced
    // it. A "point \"shader\" (0, 0, 1)" parameter is stated in that space
    surfacePlacement_ = coordinateSystems_["camera"] * transform_.reference();
}

void RenderContext::lightSource(const v3d::render::offline::rib::LightSource & request,
    const v3d::render::offline::rib::ParameterList & parameters) {
    LightSource light;
    light.handle = request.handle;
    light.shader = shaders_->instance(request.shader, v3d::render::offline::sl::ShaderType::LIGHT, parameters);
    light.placement = coordinateSystems_["camera"] * transform_.reference();
    if (!light.shader) {
        // the library has already logged why. The light is left out, not replaced by a
        // light of some other kind
        return;
    }
    lights_.push_back(light);
    // RiLightSource switches the light it creates on
    illuminate(request.handle, true);
}

void RenderContext::illuminate(const std::string & handle, bool on) {
    const std::vector<std::string>::iterator found = std::find(lit_.begin(), lit_.end(), handle);
    if (on && found == lit_.end()) {
        lit_.push_back(handle);
    } else if (!on && found != lit_.end()) {
        lit_.erase(found);
    }
}

void RenderContext::imager(const std::string & name,
    const v3d::render::offline::rib::ParameterList & parameters) {
    imager_ = shaders_->instance(name, v3d::render::offline::sl::ShaderType::IMAGER, parameters);
}

void RenderContext::searchpath(const std::string & path) {
    shaders_->searchpath(path);
}

v3d::render::offline::Textures & RenderContext::textures() {
    return *textures_;
}

Shading RenderContext::shading() {
    Shading state;
    state.surface = surface_;
    state.placement = surfacePlacement_;
    state.opacity = opacity_;
    if (!state.surface) {
        // a scene that names no surface draws "constant", which does no shading. RI leaves
        // the default surface to the renderer
        state.surface = shaders_->instance("constant",
            v3d::render::offline::sl::ShaderType::SURFACE,
            v3d::render::offline::rib::ParameterList());
        state.placement = coordinateSystems_["camera"] * transform_.reference();
    }
    for (const LightSource & light : lights_) {
        if (std::find(lit_.begin(), lit_.end(), light.handle) == lit_.end()) {
            continue;
        }
        v3d::render::offline::sl::Placed shining;
        shining.shader = light.shader;
        shining.placement = light.placement;
        state.lights.push_back(shining);
    }
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
    return traced_;
}

const v3d::render::offline::trace::Lights & RenderContext::tracedLights(const Shading & state) {
    if (tracedLights_ && tracedFor_ == lit_ && tracedLightsFor_ == lights_.size()) {
        return tracedLights_;
    }
    // a light's placement takes it to camera space, and the traced scene is in world space
    const glm::mat4x4 toWorld = glm::inverse(coordinateSystems_["camera"]);
    auto on = boost::make_shared<std::vector<v3d::render::offline::sl::Placed> >();
    for (const v3d::render::offline::sl::Placed & light : state.lights) {
        v3d::render::offline::sl::Placed placed = light;
        placed.placement = toWorld * light.placement;
        on->push_back(placed);
    }
    tracedLights_ = on;
    tracedFor_ = lit_;
    tracedLightsFor_ = lights_.size();
    return tracedLights_;
}

void RenderContext::trace(const Polygon & poly, const Shading & state) {
    if (poly.vertexCount() < 3) {
        return;
    }
    const glm::mat4x4 & toWorld = transform_.reference();
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
            v3d::render::offline::trace::Triangle(points, color_, normals) :
            v3d::render::offline::trace::Triangle(points, color_);
        if (corners[0].hasTexCoord() && corners[1].hasTexCoord() && corners[2].hasTexCoord()) {
            triangle.st(corners[0].st(), corners[1].st(), corners[2].st());
        }
        if (corners[0].hasColor() && corners[1].hasColor() && corners[2].hasColor()) {
            triangle.colours(corners[0].color(), corners[1].color(), corners[2].color());
        }
        shade(&triangle, state);
        traced_.add(triangle, transform_);
    }
}

void RenderContext::shade(v3d::render::offline::trace::Primitive* primitive, const Shading & state) {
    // a shader's placement takes it to camera space, and the traced scene is in world space
    v3d::render::offline::sl::Placed surface;
    surface.shader = state.surface;
    surface.placement = glm::inverse(coordinateSystems_["camera"]) * state.placement;
    primitive->surface(surface);
    primitive->opacity(state.opacity);
    primitive->lights(tracedLights(state));
}

bool RenderContext::addSphere(const v3d::render::offline::rib::Sphere & shape) {
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
    // placed by the reference end of its motion, as a polygon's points are
    v3d::render::offline::trace::Sphere sphere(shape, transform_.reference(), color_);
    shade(&sphere, shading());
    traced_.add(sphere, transform_);
    return true;
}

bool RenderContext::flatMotion() {
    if (!transform_.moving() || transform_.placeable()) {
        return false;
    }
    // both ends of its motion flatten it, so there is no pose to move it from
    if (!flatMotionReported_) {
        flatMotionReported_ = true;
        logger_->get()->warn("a primitive whose motion is flat at both ends is not drawn");
    }
    return true;
}

void RenderContext::addPolygon(const boost::shared_ptr<Polygon>& poly) {
    if (flatMotion()) {
        return;
    }
    if (hider_->traces()) {
        // the ray hider sees the traced scene and nothing else, so nothing is bucketed
        trace(*poly, shading());
        return;
    }
    // a polygon is placed with the state it was submitted under, and is traced once. A
    // piece handed back by a split is already placed and keeps its parent's state
    if (!poly->placed()) {
        const Shading state = shading();
        trace(*poly, state);
        poly->place(coordinateSystems_["camera"] * transform_.reference(), color_, poly->geometricNormal(),
            state);
        poly->motion(transform_.before(coordinateSystems_["camera"]));
    }

    fillVertices(poly.get());

    v3d::type::geometry::AABBox bound = poly->bound();

    glm::vec3 bound_max = bound.max();
    glm::vec3 bound_min = bound.min();

    /*
        The bound is in object space. The placement takes it to eye space, where every
        later calculation on the polygon happens. It is the camera coordinate system, which
        holds world to camera as prepareWorld() saved it, times the object to world
        transformation. It applies as it stands, with no transpose or inverse.
    */
    const glm::mat4x4 toEye = poly->placement();
    const Bound object{.min = bound_min, .max = bound_max};
    const Bound eye = transformBound(toEye, object);
    bound_min = eye.min;
    bound_max = eye.max;

    // a moving primitive is culled by where it is at both ends of the shutter, as well as by
    // where it is stored. The size test below reads where it is stored, because a split
    // shrinks a primitive and never the distance it travels
    Bound swept = eye;
    sweep(poly->motion(), object, &swept);
    const glm::vec3 & swept_min = swept.min;
    const glm::vec3 & swept_max = swept.max;

    // hither-yon cull
    if ((swept_max[2] > far_ && swept_min[2] > far_)  // the bound is wholly beyond the far plane
        || (swept_max[2] < near_ && swept_min[2] < near_)) {  // the bound is wholly nearer than the near plane
        return;
    }

    poly->diceable(true);

    // eye plane test. A bound that spans the near plane passes the cull above. If it also
    // reaches behind the eye plane, z = 0, it cannot be projected, and is marked undiceable
    // so that it is split
    if (bound_max[2] > near_ && bound_min[2] < near_) {
        if (bound_min[2] < 0.0) {
            poly->diceable(false);
        }
    }

    // viewing frustum cull. The frustum is built from the "screen" coordinate system, which
    // takes eye space to screen space, so it is tested against the eye space bound swept
    // over the shutter. Only the bound is transformed; the vertices stay in object space
    const v3d::type::geometry::Frustum frustum(coordinateSystems_["screen"], v3d::type::geometry::Frustum::Depth::MinusOneToOne);
    v3d::type::geometry::AABBox eyeBound;
    eyeBound.extents(swept_min, swept_max);
    if (frustum.intersect(eyeBound) == v3d::type::geometry::Frustum::OUTSIDE) {
        return;
    }

    // raster applies after screen, so it is on the left: a matrix applies to what is on its
    // right
    const glm::mat4x4 screen = coordinateSystems_["raster"] * coordinateSystems_["screen"];
    // all eight corners of the eye space bound are projected, because under a perspective
    // projection the raster extremes need not lie at two diagonal corners. A corner at or
    // behind the eye has no raster position. It is left out, and the primitive is split. The
    // test on w is the one project() makes before it divides
    glm::vec3 raster_min(std::numeric_limits<float>::max());
    glm::vec3 raster_max(std::numeric_limits<float>::lowest());
    bool projected = false;
    for (int k = 0; k < 8; k++) {
        const glm::vec3 corner = boxCorner(bound_min, bound_max, k);
        if ((screen * glm::vec4(corner, 1.0F)).w <= SMALLEST_W) {
            poly->diceable(false);
            continue;
        }
        const glm::vec3 point = project(screen, corner);
        raster_min = glm::min(raster_min, point);
        raster_max = glm::max(raster_max, point);
        projected = true;
    }
    if (!projected) {
        // no corner is in front of the eye. The primitive is split, and its pieces are
        // bucketed again, so the first bucket holds it until then
        raster_min = glm::vec3(0.0F);
        raster_max = glm::vec3(0.0F);
    }
    bound_min = raster_min;
    bound_max = raster_max;

    bound.extents(bound_min, bound_max);

    // a primitive already marked for splitting needs no size test
    if (poly->diceable()) {
        // a raster bound wider or taller than one grid's side, sqrt(gridSize_) *
        // shadingRate_ pixels, is split
        float size = static_cast<float>(gridSize_);
        size = sqrt(size) * shadingRate_;
        glm::vec3 bound_size = bound_max - bound_min;
        if ((bound_size[0] > size) || (bound_size[1] > size)) {
            poly->diceable(false);
        }
    }

    // dicing is done in eye space, so a polygon that will be diced has its vertices moved
    // there now
    if (poly->diceable()) {
        // a normal transforms by the inverse transpose of the matrix that moves the points.
        // The two agree under a rotation and a uniform scale. Under a scale on one axis the
        // point matrix would tilt a normal off its surface
        const glm::mat3x3 toEyeNormal = glm::transpose(glm::inverse(glm::mat3x3(toEye)));
        for (unsigned int i = 0; i < poly->vertexCount(); i++) {
            Vertex pv = poly->vertex(i);
            pv.point(glm::vec3(toEye * glm::vec4(pv.point(), 1.0F)));
            pv.normal(glm::normalize(toEyeNormal * pv.normal()));
            pv.geometricNormal(glm::normalize(toEyeNormal * pv.geometricNormal()));
            (*poly)[i] = pv;
        }
    }

    // the last step of the first pass: the polygon goes in the bucket that holds the upper
    // left corner of its raster bound
    frameBuffer_->addPrimitive(poly, bound);
}

void RenderContext::display(const v3d::render::offline::rib::Display & display) {
    displayName_ = display.name;
    displayType_ = display.type;
    displayMode_ = display.mode;
}

const std::string & RenderContext::displayName() const {
    return displayName_;
}

void RenderContext::bucket(v3d::render::offline::FrameBuffer* planes) {
    samples_ = boost::make_shared<reyes::Samples>(glm::uvec2(planes->width(), planes->height()), sampling_);
    frameBuffer_->render(*this);
    samples_->resolve(planes, FrameBuffer::RED, FrameBuffer::COVERAGE, FrameBuffer::DEPTH);
}

void RenderContext::render() {
    if (!frameBuffer_) {
        return;
    }

    const boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = frameBuffer_->planes();
    hider_->render(this, planes.get());

    if (imager_) {
        // the imager runs once the hider has finished, because it reads the whole picture.
        // The shader's machine logs a failed run. The failed row and the rows below it stay
        // as the hider wrote them, and the frame is still written
        v3d::render::offline::sl::Imager imager(imager_, &shader());
        imager.run(frameBuffer_->planes().get(), FrameBuffer::COVERAGE);
    }

    // a display named nothing, or one that is not a file, leaves the samples in the
    // planes rather than writing them
    if (displayName_.empty() || displayType_ != "file") {
        return;
    }

    // only the three colour channels are written, whatever the display mode. The coverage
    // and depth planes stay in the framebuffer
    auto logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    factory.write(displayName_, frameBuffer_->planes()->image(FrameBuffer::CHANNELS));
}

};  // namespace v3d::moya
