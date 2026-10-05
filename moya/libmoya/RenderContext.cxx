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

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include "GridShader.h"
#include "Hider.h"
#include "RayHider.h"

namespace v3d::moya {

glm::vec3 project(const glm::mat4x4 & m, const glm::vec3 & point) {
    const glm::vec4 projected = m * glm::vec4(point, 1.0f);
    if (projected.w <= 1.0e-6f) {
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
    hider_ = boost::make_shared<ReyesHider>();
    shaders_ = boost::make_shared<v3d::render::offline::sl::ShaderLibrary>(logger_);
    textures_ = boost::make_shared<v3d::render::offline::Textures>(logger_);

    // initialize the predefined coordinate systems to defaults (identity matrix)
    glm::mat4x4 def(1.0f);
    coordinateSystems_["object"] = def;
    coordinateSystems_["world"] = def;
    coordinateSystems_["camera"] = def;
    coordinateSystems_["screen"] = def;
    coordinateSystems_["raster"] = def;
    coordinateSystems_["NDC"] = def;


    // initialize the z buffer

    // initialize buckets - done in prepareWorld()
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

Samples & RenderContext::samples() {
    assert(samples_);
    return *samples_;
}

v3d::render::offline::Sampling & RenderContext::sampling() {
    return sampling_;
}

const v3d::render::offline::Sampling & RenderContext::sampling() const {
    return sampling_;
}

unsigned int RenderContext::samplesTaken(unsigned int column, unsigned int row) const {
    return hider_->samplesTaken(column, row);
}

/*
    maps to RiWorldBegin()
    freezes all rendering options, world to camera transformation
    is set to current transormation, current transformation is set to identity
    the view options are frozen beyond this call
    the framebuffer will be allocated here
*/
void RenderContext::prepareWorld() {
    // a scene that named no projection gets the default, and one that named a projection
    // has already had its screen transform built and its transform reset - projecting
    // again here would compose the projection twice and would save that reset as the
    // camera transform
    if (!projectionNamed_) {
        projection("");
    }

    // establish the world coordinate system
    // save the existing transform as the camera coordinate system. What a scene set
    // between RiProjection and here is the world to camera transformation
    saveCoordinateSystem("camera");
    // a traced hit's "camera" space and its E are this camera's, because the traced scene
    // has no camera of its own
    traced_.view(coordinateSystems_["camera"]);
    // inside the world block the current transformation is object to world
    transform_.replace(glm::mat4x4(1.0f));

    // set raster transformation
    glm::mat4x4 raster(1.0f);  // identity
    /*
        screen transform maps to the canonical volume [-1, 1]
        raster transform scales to [0, xres] x [0, yres]

        y is negated because raster space has its origin at the upper left corner and
        counts downward, which is also the row order image::Image is in - without the
        flip a correctly computed picture is written upside down.
    */
    float x = xres_ / 2.0f;
    float y = yres_ / 2.0f;
    raster = glm::scale(raster, glm::vec3(x, -y, 1.0f));
    raster = glm::translate(raster, glm::vec3(1.0f, -1.0f, 1.0f));

    // mark the raster coordinate system
    coordinateSystems_["raster"] = raster;

    unsigned int screen[2];
    screen[0] = xres_;
    screen[1] = yres_;
    unsigned int bucket[2];
    bucket[0] = bucketWidth_;
    bucket[1] = bucketHeight_;

    frameBuffer_.reset(new FrameBuffer(bucket, screen));
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

/*
set a named projection transformation matrix
the combination of the projection and screen transformation matrices
move between camera and screen coordinate space
*/
bool RenderContext::perspective() const {
    return projection_ == "perspective";
}

void RenderContext::projection(std::string name, float fov) {
    if (name.empty()) {
        name = "orthographic";
    }

    // name is orthographic, perspective, or empty
    // only perspective uses fov
    projection_ = name;
    fov_ = fov;
    projectionNamed_ = true;

    const float left = screen_[0];
    const float right = screen_[1];
    const float bottom = screen_[2];
    const float top = screen_[3];

    // an unsupported projection falls through every branch below, so this has to start as
    // something composable rather than as whatever the stack held
    glm::mat4x4 projection(1.0f);
    // build the projection matrix
    if (name == "perspective") {
        /*
            RI states fov as the full angle between screen space (-1, 0) and (1, 0), so a
            point at eye depth z reaches screen x = 1 at x = z * tan(fov / 2). The screen
            window then selects the part of screen space the image covers.

            The interface looks down +z, so w is +z rather than the -z a right handed
            system would write, and depth runs [-1, 1] to match the orthographic branch,
            which is the depth range the cull below names to its Frustum.
        */
        const float tangent = std::tan(glm::radians(fov) / 2.0f);
        projection = glm::mat4x4(0.0f);
        projection[0][0] = 2.0f / ((right - left) * tangent);
        projection[1][1] = 2.0f / ((top - bottom) * tangent);
        projection[2][0] = -(right + left) / (right - left);
        projection[2][1] = -(top + bottom) / (top - bottom);
        projection[2][2] = (far_ + near_) / (far_ - near_);
        projection[2][3] = 1.0f;
        projection[3][2] = -2.0f * far_ * near_ / (far_ - near_);
    } else if (name == "orthographic") {
        /*
            [2 / (right-left)	0					0				-tx	]
            [0					2 / (bottom-top)	0				-ty	]
            [0					0					2/(far-near)	-tz	]
            [0					0					0				1	]

            tx = (right + left) / (right - left)
            ty = (top + bottom) / (top - bottom)
            tz = (far + near) / (far - near)
        [  0,  1,  2,  3 ]
        [  4,  5,  6,  7 ]
        [  8,  9, 10, 11 ]
        [ 12, 13, 14, 15 ]

        m[0] = 2 / screen window width
        m[5] = 2 / screen window height

        glm (column-major ordering):
        [  00,  10,  20,  30 ]
        [  01,  11,  21,  31 ]
        [  02,  12,  22,  32 ]
        [  03,  13,  23,  33 ]
        */

        float far = far_;
        float near = near_;

        float tx = ((right + left) / (right - left));
        float ty = ((top + bottom) / (top - bottom));
        float tz = (far + near) / (far - near);

        projection[0][0] = 2.0f / (right - left);
        projection[1][0] = 0.0;
        projection[2][0] = 0.0;
        projection[3][0] = -tx;
        projection[0][1] = 0.0;
        projection[1][1] = 2.0f / (top - bottom);
        projection[2][1] = 0.0;
        projection[3][1] = -ty;
        projection[0][2] = 0.0;
        projection[1][2] = 0.0;
        // positive, as the matrix above is written: the interface looks down +z, and the
        // negated form belongs to a right handed system, where it puts the whole of the
        // clip range behind the near plane
        projection[2][2] = 2.0f / (far - near);
        projection[3][2] = -tz;
        projection[0][3] = 0.0;
        projection[1][3] = 0.0;
        projection[2][3] = 0.0;
        projection[3][3] = 1.0;

        // std::cerr << projection;
    } else {  // implementation-specific projection
        // unsupported projections default to orthographic
    }

    // append the projection to the current transformation. RI states the composition in
    // row vectors, where the projection is on the right; a matrix applies to what is on
    // its right here, so it goes on the left
    transform_ = transform_.before(projection);
    // save as screen coordinate system
    saveCoordinateSystem("screen");

    // reinitialize current transformation to indentity matrix
    transform_.replace(glm::mat4x4(1.0f));
    // current transformation matrix is now the camera coordinate system
}

void RenderContext::hider(const std::string & name) {
    if (name == "hidden") {
        hider_ = boost::make_shared<ReyesHider>();
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

/*
    maps to RiFormat(xres, yres, aspect)
    sets the pixel resolution and aspect ratio of the image to be rendered
    default values will be used when not called
*/
void RenderContext::imageResolution(int xres, int yres, float aspect) {
    xres_ = xres;
    yres_ = yres;
    pixelAspect_ = aspect;
    if (!frameAspectNamed_ && yres_ > 0) {
        frameAspectRatio(xres_ * pixelAspect_ / yres_);
        frameAspectNamed_ = false;
    }
}

/*
    maps to RiFrameAspectRatio(aspect)
*/
void RenderContext::frameAspectRatio(float aspect) {
    frameAspect_ = aspect;
    frameAspectNamed_ = true;
    if (screenNamed_) {
        return;
    }
    // the RI default: the wider dimension spans [-1, 1] and the other is the reciprocal,
    // so that a frame which is not square does not stretch a square window across itself
    if (frameAspect_ >= 1.0f) {
        screenWindow(-frameAspect_, frameAspect_, -1.0f, 1.0f);
    } else {
        screenWindow(-1.0f, 1.0f, -1.0f / frameAspect_, 1.0f / frameAspect_);
    }
    screenNamed_ = false;
}

/*
    maps to RiScreenWindow(left, right, bottom, top)
*/
void RenderContext::screenWindow(float left, float right, float bottom, float top) {
    screen_[0] = left;
    screen_[1] = right;
    screen_[2] = bottom;
    screen_[3] = top;
    screenNamed_ = true;
}

/*
    maps to RiClipping(hither, yon)
    sets the position of the near and far clipping planes
*/
void RenderContext::clipping(float near, float far) {
    near_ = near;
    far_ = far;
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
    // which lights are on comes back; the lights themselves do not, because a light
    // belongs to the frame rather than to the block that created it
    lit_ = saved.lit;
    attributes_.pop_back();
}

void RenderContext::saveCoordinateSystem(const std::string& name) {
    coordinateSystems_[name] = transform_.reference();
}

void RenderContext::setCoordinateSystem(const std::string& name) {
    // make sure name is a valid coordinate system

    transform_.replace(coordinateSystems_[name]);
}

void RenderContext::setIdentityTransform() {
    transform_.replace(glm::mat4(1.0f));
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
    bucketWidth_ = width;
    bucketHeight_ = height;
}

void RenderContext::gridSize(unsigned int size) {
    gridSize_ = size;
}

glm::mat4x4 RenderContext::coordinateSystem(const std::string& name) {
    return coordinateSystems_[name];
}

void RenderContext::translate(float dx, float dy, float dz) {
    transform_.concat(glm::translate(glm::mat4x4(1.0f), glm::vec3(dx, dy, dz)));
}

// RiRotate takes its angle in degrees and glm takes radians
void RenderContext::rotate(float angle, float dx, float dy, float dz) {
    transform_.concat(glm::rotate(glm::mat4x4(1.0f), glm::radians(angle), glm::vec3(dx, dy, dz)));
}

void RenderContext::scale(float sx, float sy, float sz) {
    transform_.concat(glm::scale(glm::mat4x4(1.0f), glm::vec3(sx, sy, sz)));
}

/*
    maps to RiPolygon()
    this covers the initial pass of the reyes architecture
*/
namespace {

/**
 * Put a bound's two corners back the right way round.
 *
 * A transform reverses an axis whenever it scales it negatively or turns the box past a
 * right angle, and the corner named min then holds the larger value on that axis.
 **/
void orderBound(glm::vec3* min, glm::vec3* max) {
    for (glm::length_t axis = 0; axis < 3; axis++) {
        if ((*min)[axis] > (*max)[axis]) {
            std::swap((*min)[axis], (*max)[axis]);
        }
    }
}

};  // namespace

void RenderContext::surface(const std::string & name,
    const v3d::render::offline::rib::ParameterList & parameters) {
    surface_ = shaders_->instance(name, v3d::render::offline::sl::ShaderType::SURFACE, parameters);
    // RI says a shader's own space is the transform in force when the scene instanced it,
    // and a "point \"shader\" (0, 0, 1)" in it is stated against that space
    surfacePlacement_ = coordinateSystems_["camera"] * transform_.reference();
}

void RenderContext::lightSource(const std::string & name, const std::string & handle,
    const v3d::render::offline::rib::ParameterList & parameters) {
    LightSource light;
    light.handle = handle;
    light.shader = shaders_->instance(name, v3d::render::offline::sl::ShaderType::LIGHT, parameters);
    light.placement = coordinateSystems_["camera"] * transform_.reference();
    if (!light.shader) {
        // the library has already logged why. A light that will not compile is left out
        // rather than replaced by a light of some other kind
        return;
    }
    lights_.push_back(light);
    // RiLightSource creates the light and switches it on, so a scene that wants one light
    // needs one request
    illuminate(handle, true);
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
        // a scene that names no surface draws the shader that means no shading. RI leaves
        // the default to the renderer and forbids only "null"
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

GridShader & RenderContext::shader() {
    if (!shader_) {
        shader_ = boost::make_shared<GridShader>(this);
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
    // a light is placed in camera space, which is moya's current space, and the traced
    // scene is in world space
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
        glm::vec3 points[3];
        for (int k = 0; k < 3; k++) {
            points[k] = glm::vec3(toWorld * glm::vec4(corners[k].point(), 1.0f));
        }
        const bool normals = corners[0].hasNormal() && corners[1].hasNormal() && corners[2].hasNormal();
        v3d::render::offline::trace::Triangle triangle = normals ?
            v3d::render::offline::trace::Triangle(points[0], points[1], points[2], color_,
                glm::normalize(toWorldNormal * corners[0].normal()),
                glm::normalize(toWorldNormal * corners[1].normal()),
                glm::normalize(toWorldNormal * corners[2].normal())) :
            v3d::render::offline::trace::Triangle(points[0], points[1], points[2], color_);
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
    // a shader is placed in camera space, which is moya's current space, and the traced
    // scene is in world space
    v3d::render::offline::sl::Placed surface;
    surface.shader = state.surface;
    surface.placement = glm::inverse(coordinateSystems_["camera"]) * state.placement;
    primitive->surface(surface);
    primitive->opacity(state.opacity);
    primitive->lights(tracedLights(state));
}

bool RenderContext::addSphere(float radius, float zmin, float zmax, float thetamax) {
    if (!hider_->traces()) {
        return false;
    }
    if (!(radius > 0.0f)) {
        logger_->get()->warn("a sphere of radius {} is not drawn", radius);
        return true;
    }
    // placed by the reference end of its motion, as a polygon's points are
    v3d::render::offline::trace::Sphere sphere(radius, zmin, zmax, thetamax, transform_.reference(), color_);
    shade(&sphere, shading());
    traced_.add(sphere, transform_);
    return true;
}

void RenderContext::addPolygon(const boost::shared_ptr<Polygon>& poly) {
    // if an output stream exists
    // echo RiPolygon RIB command to output stream

    // bound polygon in eye space
    /*
        the polygon's vertices are in object space, so its bound is too. The placement
        carries both into eye space, where every later calculation on the polygon happens.
    */
    // a primitive carries the state it was submitted under - see ReyesPrimitive::place().
    // A piece handed back by a split is already placed and keeps its parent's
    if (hider_->traces()) {
        // the ray hider sees the traced scene and nothing else, so nothing is bucketed
        trace(*poly, shading());
        return;
    }
    if (!poly->placed()) {
        const Shading state = shading();
        trace(*poly, state);
        poly->place(coordinateSystems_["camera"] * transform_.reference(), color_, poly->geometricNormal(),
            state);
        poly->motion(transform_.before(coordinateSystems_["camera"]));
    }

    // a vertex that brought no "Cs" of its own takes the primitive's colour, which the
    // surface shader then reads as Cs.
    //
    // The normals go the same way: Ng is the primitive's plane on every vertex, and a
    // vertex that brought no varying "N" shades with it, so a polygon that gives no
    // normals is shaded faceted
    for (unsigned int i = 0; i < poly->vertexCount(); i++) {
        if (!(*poly)[i].hasColor()) {
            (*poly)[i].color(poly->color());
        }
        (*poly)[i].geometricNormal(poly->normal());
        if (!(*poly)[i].hasNormal()) {
            (*poly)[i].normal(poly->normal());
        }
    }

    v3d::type::geometry::AABBox bound = poly->bound();

    glm::vec3 bound_max = bound.max();
    glm::vec3 bound_min = bound.min();

    /*
        convert bound to eye space coordinates: the modeling transformation takes it to
        world coordinates, and the camera transformation takes that to eye coordinates.
        The primitive's placement holds the two composed.
    */

    // the camera coordinate system holds the world to camera transformation, which is what
    // prepareWorld() saved, so it applies as it stands. Neither a transpose nor an inverse
    // belongs here: both happen to be right when it is a rotation and neither is when a
    // scene places its camera with a matrix that also translates
    const glm::mat4x4 toEye = poly->placement();
    const glm::vec3 objectMin = bound_min;
    const glm::vec3 objectMax = bound_max;
    bound_max = glm::vec3(toEye * glm::vec4(objectMax, 1.0f));
    bound_min = glm::vec3(toEye * glm::vec4(objectMin, 1.0f));

    // camera transform might've flipped some components of min & max
    orderBound(&bound_min, &bound_max);

    // a moving primitive is culled by where it is at either end of the shutter as well as
    // where it opened; the size test below reads where it opened, because a split shrinks a
    // primitive and never the distance it travels
    glm::vec3 swept_min = bound_min;
    glm::vec3 swept_max = bound_max;
    if (poly->motion().moving()) {
        glm::vec3 closeMin(poly->motion().close() * glm::vec4(objectMin, 1.0f));
        glm::vec3 closeMax(poly->motion().close() * glm::vec4(objectMax, 1.0f));
        orderBound(&closeMin, &closeMax);
        swept_min = glm::min(swept_min, closeMin);
        swept_max = glm::max(swept_max, closeMax);
    }

    // do hither-yon cull
    if ((swept_max[2] > far_ && swept_min[2] > far_)  // bound is completely outside far plane (too far away for the camera to see)
        || (swept_max[2] < near_ && swept_min[2] < near_)) {  // bound is completely outside near plane (effectively behind camera)
        // cull poly
        // no need to continue since poly won't be rendered
        return;
    }

    // set initial diceable state of the primitive
    poly->diceable(true);

    // do hither e plane test & mark undiceable if necessary
    /*
        anything that spans hither or yon will make it past the first cull step above.
        those conditions are:
        (bound_max[2] < far_ && bound_min[2] > far_)	not possible. max > min
        (bound_max[2] > far_ && bound_min[2] < far_)	crosses far plane
        (bound_max[2] > near_ && bound_min[2] < near_)	crosses near plane
        (bound_max[2] < near_ && bound_min[2] > near_)	not possible. max > min

        in the e plane test we are only concerned with the third condition
    */
    if (bound_max[2] > near_ && bound_min[2] < near_) {  // crosses near plane
        // if bound_min[2] also crosses the eye plane, we'll need to split it
        // (mark undiceable so it will be split on the next pass)
        if (bound_min[2] < 0.0) {
            poly->diceable(false);
        }
        // else poly is in front of the eye plane and still projectable so continue
    }

    // convert bounds to screen space
    /*
        to get from eye to screen space apply the projection and screen transformations
        the projection matrix is appended to the current transformation when RiProjection() is called
        the current transformation is then saved as the screen transformation (with original screen &
        projection matrices already combined).
        we don't want to transform the primitive here, just the bounds.
        we'll still need eye space in the second pass.
    */
    // poly->transform(coordinateSystems_["screen"]);
    // screen space to reyes means renderman raster space

    // do viewing frustum cull
    /*
        The planes of a matrix bound the region of the space it reads that lands inside the
        canonical clip volume, so the test is against the eye space bound and against the
        projection alone. Testing the raster space bound against the planes of the raster
        matrix asks whether pixel coordinates fall inside a volume measured in eye units.
    */
    const v3d::type::geometry::Frustum frustum(coordinateSystems_["screen"], v3d::type::geometry::Frustum::Depth::MinusOneToOne);
    v3d::type::geometry::AABBox eyeBound;
    eyeBound.extents(swept_min, swept_max);
    if (frustum.intersect(eyeBound) == v3d::type::geometry::Frustum::OUTSIDE) {  // poly is entirely outside frustum
        return;
    }

    // bound = poly->bound();
    // the raster transform reads the canonical volume the projection writes, so it goes
    // on the left - a matrix applies to what is on its right
    glm::mat4x4 screen = coordinateSystems_["raster"] * coordinateSystems_["screen"];
    // two corners through a perspective projection bound the box only approximately - the
    // eight are not the two once w varies - which is enough for the size test and the
    // bucket this picks, and is what the split below re-measures anyway
    bound_min = project(screen, bound_min);
    bound_max = project(screen, bound_max);

    // screen transform might've flipped some components of min & max
    orderBound(&bound_min, &bound_max);

    bound.extents(bound_min, bound_max);

    /*
        if the primitive is spanning the e plane then we already know it needs split
        but we still need to determine if any other primitive that hasn't already
        been culled will need to be split.
    */
    if (poly->diceable()) {
        /*
            if our primitive's raster space bound is bigger
            than the maximum grid size then it should be split
        */
        float size = static_cast<float>(gridSize_);
        size = sqrt(size) * shadingRate_;
        glm::vec3 bound_size = bound_max - bound_min;
        if ((bound_size[0] > size) || (bound_size[1] > size)) {
            poly->diceable(false);
        }
    }

    /*
        if poly is diceable then we don't need to split it
        dicing is done in eye space so we go ahead and transform the poly's
        vertices now.
     */
    if (poly->diceable()) {
        // a normal transforms by the inverse transpose rather than by the matrix that
        // moves the points. The two agree under a rotation and a uniform scale, and differ
        // as soon as a scene scales one axis, which would tilt a normal off its surface
        const glm::mat3x3 toEyeNormal = glm::transpose(glm::inverse(glm::mat3x3(toEye)));
        for (unsigned int i = 0; i < poly->vertexCount(); i++) {
            Vertex pv = poly->vertex(i);
            pv.point(glm::vec3(toEye * glm::vec4(pv.point(), 1.0f)));
            pv.normal(glm::normalize(toEyeNormal * pv.normal()));
            pv.geometricNormal(glm::normalize(toEyeNormal * pv.geometricNormal()));
            (*poly)[i] = pv;
        }
    }

    // put the polygon in a bucket
    /*
        this is the last step of the first pass
        we just need to know which bucket to put the primitive in
        to do this we need the upper left corner of the primitive's bound
        buckets work in raster space (buckets are evenly tiled in pixel space)
        up to now our bound is only in screen space

        NOTE: reyes does not have raster or ndc space. raster space is the same as screen space
              so when we move to screen space above, we should really be moving to what
              RenderMan calls raster space.
    */
    frameBuffer_->addPrimitive(poly, bound);
    // nothing left to do in the first pass for this primitive
}
/*
    maps to RiWorldEnd()
    once rendering is done, objects, lights and other stuff set
    after prepareWorld() are destroyed and the memory reclaimed
    saving the output file is done here also.
    the results might not even include a rendered image depending
    on the context. we might just output a rib file.
*/
/*
    maps to RiDisplay()
*/
void RenderContext::display(const std::string & name, const std::string & type, const std::string & mode) {
    displayName_ = name;
    displayType_ = type;
    displayMode_ = mode;
}

const std::string & RenderContext::displayName() const {
    return displayName_;
}

void RenderContext::bucket(v3d::render::offline::FrameBuffer* planes) {
    samples_ = boost::make_shared<Samples>(planes->width(), planes->height(), sampling_);
    frameBuffer_->render(*this);
    samples_->resolve(planes, FrameBuffer::RED, FrameBuffer::COVERAGE, FrameBuffer::DEPTH);
}

/*
    hide what the world gathered, then write what it sampled
*/
void RenderContext::render() {
    if (!frameBuffer_) {
        return;
    }

    boost::shared_ptr<v3d::render::offline::FrameBuffer> planes = frameBuffer_->planes();
    hider_->render(this, planes.get());

    if (imager_) {
        // after the last bucket, once every sample of the frame is in: an imager is a
        // function of the finished picture rather than of a piece
        v3d::render::offline::sl::Imager imager(imager_, &shader());
        imager.run(frameBuffer_->planes().get(), FrameBuffer::COVERAGE);
    }

    // a display named nothing, or one that is not a file, leaves the samples in the
    // planes rather than writing them
    if (displayName_.empty() || displayType_ != "file") {
        return;
    }

    // every display mode writes the three colour channels. The coverage and depth planes
    // are resolved, and are not written to the file
    auto logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    factory.write(displayName_, frameBuffer_->planes()->image(FrameBuffer::CHANNELS));
}

};  // namespace v3d::moya
