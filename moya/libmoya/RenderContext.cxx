/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderContext.h"

#include <api/image/Factory.h>

#include <algorithm>
#include <cmath>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat3x3.hpp>
#include <glm/matrix.hpp>

#include "Frustum.h"
#include "GridShader.h"

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
    shaders_ = boost::make_shared<v3d::render::offline::sl::ShaderLibrary>(logger_);

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
    // inside the world block the current transformation is object to world
    transform_ = glm::mat4x4(1.0f);

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
void RenderContext::projection(std::string name, float fov) {
    if (name.empty()) {
        name = "orthographic";
    }

    // name is orthographic, perspective, or empty
    // only perspective uses fov
    projection_ = name;
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
            system would write, and depth runs [-1, 1] to match the orthographic branch
            and the plane extraction in Frustum::extract.
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
    transform_ = projection * transform_;
    // save as screen coordinate system
    saveCoordinateSystem("screen");

    // reinitialize current transformation to indentity matrix
    transform_ = glm::mat4x4(1.0f);
    // current transformation matrix is now the camera coordinate system
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
    coordinateSystems_[name] = transform_;
}

void RenderContext::setCoordinateSystem(const std::string& name) {
    // make sure name is a valid coordinate system

    transform_ = coordinateSystems_[name];
}

void RenderContext::setIdentityTransform() {
    transform_ = glm::mat4(1.0f);
}

void RenderContext::setTransform(const glm::mat4x4& trans) {
    transform_ = trans;
}

void RenderContext::concatTransform(const glm::mat4x4& trans) {
    transform_ = transform_ * trans;
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
    transform_ = glm::translate(transform_, glm::vec3(dx, dy, dz));
}

// RiRotate states its angle in degrees, which is the one place the interface disagrees
// with glm
void RenderContext::rotate(float angle, float dx, float dy, float dz) {
    transform_ = glm::rotate(transform_, glm::radians(angle), glm::vec3(dx, dy, dz));
}

void RenderContext::scale(float sx, float sy, float sz) {
    transform_ = glm::scale(transform_, glm::vec3(sx, sy, sz));
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
    // which is what a "point \"shader\" (0, 0, 1)" in it is stated against
    surfacePlacement_ = coordinateSystems_["camera"] * transform_;
}

void RenderContext::lightSource(const std::string & name, const std::string & handle,
    const v3d::render::offline::rib::ParameterList & parameters) {
    LightSource light;
    light.handle = handle;
    light.shader = shaders_->instance(name, v3d::render::offline::sl::ShaderType::LIGHT, parameters);
    light.placement = coordinateSystems_["camera"] * transform_;
    if (!light.shader) {
        // the library has already said why, and a light that will not compile is one
        // fewer light rather than a light of some other kind
        return;
    }
    lights_.push_back(light);
    // RiLightSource creates the light and switches it on, which is why this is not two
    // requests in a scene that wants one light
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

Shading RenderContext::shading() {
    Shading state;
    state.surface = surface_;
    state.placement = surfacePlacement_;
    state.opacity = opacity_;
    if (!state.surface) {
        // a scene that names no surface draws the shader that means no shading, which is
        // the picture this renderer drew before there was a language. RI leaves the
        // default to the renderer and forbids only "null"
        state.surface = shaders_->instance("constant",
            v3d::render::offline::sl::ShaderType::SURFACE,
            v3d::render::offline::rib::ParameterList());
        state.placement = coordinateSystems_["camera"] * transform_;
    }
    for (const LightSource & light : lights_) {
        if (std::find(lit_.begin(), lit_.end(), light.handle) == lit_.end()) {
            continue;
        }
        Light shining;
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

void RenderContext::addPolygon(boost::shared_ptr<Polygon> poly) {
    // if an output stream exists
    // echo RiPolygon RIB command to output stream

    // bound polygon in eye space
    /*
        if this is just based off of poly vertices, the bound will be in object space
        we'll need to apply the current modeling transformation to get from object space to world space
        and then the camera transformation will need to be applied to get into eye space
        we should probably just transform the poly to eye space first since any future calculations
        on this poly will be done in eye space or beyond.
    */
    // a primitive carries the state it was submitted under - see ReyesPrimitive::place().
    // A piece handed back by a split is already placed and keeps its parent's
    if (!poly->placed()) {
        poly->place(coordinateSystems_["camera"] * transform_, color_, poly->geometricNormal(),
            shading());
    }

    // a vertex that brought no "Cs" of its own takes the primitive's colour. There is no
    // light and no material behind it - RiSurface is still empty - so this is the
    // geometry's colour rather than a shaded one.
    //
    // The normals go the same way: Ng is the primitive's plane on every vertex, and a
    // vertex that brought no varying "N" shades with it, which is what makes a polygon
    // that says nothing about its normals faceted
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
        convert bound to eye space coordinates
        first multiply by modeling transformation to get world coordinates
        next multiply by camera transformation to get camera/eye coordinates

        for now we're just taking the current transform.
        later we'll probably need to concatenate the transforms_ matrix stack too
    */

    // the camera coordinate system holds the world to camera transformation, which is what
    // prepareWorld() saved, so it applies as it stands. Neither a transpose nor an inverse
    // belongs here: both happen to be right when it is a rotation and neither is when a
    // scene places its camera with a matrix that also translates
    const glm::mat4x4 toEye = poly->placement();
    bound_max = glm::vec3(toEye * glm::vec4(bound_max, 1.0f));
    bound_min = glm::vec3(toEye * glm::vec4(bound_min, 1.0f));

    // camera transform might've flipped some components of min & max
    orderBound(&bound_min, &bound_max);

    // do hither-yon cull
    if ((bound_max[2] > far_ && bound_min[2] > far_)  // bound is completely outside far plane (too far away for the camera to see)
        || (bound_max[2] < near_ && bound_min[2] < near_)) {  // bound is completely outside near plane (effectively behind camera)
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
    Frustum frustum(coordinateSystems_["screen"]);
    v3d::type::geometry::AABBox eyeBound;
    eyeBound.extents(bound_min, bound_max);
    if (frustum.intersect(eyeBound) == Frustum::OUTSIDE) {  // poly is entirely outside frustum
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
        // moves the points. The two agree under a rotation and a uniform scale, and part
        // company the moment a scene scales one axis, which tilts a normal off its surface
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

/*
    perform the second reyes pass, then write what it sampled
*/
void RenderContext::render() {
    if (!frameBuffer_) {
        return;
    }

    frameBuffer_->render(*this);

    // a display named nothing, or one that is not a file, leaves the samples in the
    // planes rather than writing them
    if (displayName_.empty() || displayType_ != "file") {
        return;
    }

    // the alpha and depth modes need planes the hider does not write yet, so every mode
    // is the three colour channels for now
    auto logger = boost::make_shared<v3d::log::Logger>();
    v3d::image::Factory factory(logger);
    factory.write(displayName_, frameBuffer_->planes()->image(FrameBuffer::CHANNELS));
}

};  // namespace v3d::moya
