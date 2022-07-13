/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "RenderContext.h"

#include <cmath>
#include <iostream>
#include <utility>

#include <glm/gtc/matrix_transform.hpp>

#include "Frustum.h"

#include "../../v3dlibs/type/3dtypes.h"

namespace v3d::moya {

    RenderContext::RenderContext() : xres_(320), yres_(240), pixelAspect_(1.0),
        projection_("orthographic"), shutterOpen_(0.0), shutterClose_(0.0),
        bucketWidth_(16), bucketHeight_(16), gridSize_(256), shadingRate_(1.0) {
        initialize();
    }

    RenderContext::RenderContext(const std::string& name) : name_(name), xres_(320), yres_(240), pixelAspect_(1.0),
        projection_("orthographic"), shutterOpen_(0.0), shutterClose_(0.0),
        bucketWidth_(16), bucketHeight_(16), gridSize_(256), shadingRate_(1.0) {
        initialize();
    }

    RenderContext::~RenderContext() {
    }

    void RenderContext::initialize() {
        // initialize the predefined coordinate systems to defaults (identity matrix)
        glm::mat4x4 def(1.0f);
        coordinateSystems_["object"] = def;
        coordinateSystems_["world"] = def;
        coordinateSystems_["camera"] = def;
        coordinateSystems_["screen"] = def;
        coordinateSystems_["raster"] = def;
        coordinateSystems_["NDC"] = def;

        transform_ = glm::mat4x4(1.0f);  //  identity

        // initialize the z buffer

        // initialize buckets - done in prepareWorld()
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
        // do we always need to do this or only when a projection hasn't explicitly been set?
        projection("");

        // establish the world coordinate system
        // save the existing transform as the camera coordinate system
        saveCoordinateSystem("camera");

        // set raster transformation
        glm::mat4x4 raster(1.0f);  // identity
        /*
            screen transform maps to the canonical volume [-1, 1]
            raster transform scales to [0, xres] x [0, yres]
        */
        float x = xres_ / 2.0f;
        float y = yres_ / 2.0f;
        glm::scale(raster, glm::vec3(x, y, 1.0f));
        glm::translate(raster, glm::vec3(1.0f, 1.0f, 1.0f));

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
        if (name.size() == 0) {
            name = "orthographic";
        }

        // name is orthographic, perspective, or empty
        // only perspective uses fov
        projection_ = name;

        glm::mat4x4 projection;
        // build the projection matrix
        if (name == "perspective") {
            projection = glm::mat4x4(1.0f);  // identity
        } else if (name == "orthographic") {
            /*
                orthographic projection looks something like:

                [ 2/(xright - xleft)	0						0					-((xright + xleft)/(xright-xleft))	]
                [ 		0				2/(ytop - ybottom)		0					-((ytop+ybottom)/(ytop-ybottom))	]
                [		0				0						-2/(zfront-zback)	(zfront+zback)/(zfront-zback)		]
                [		0				0						0					1									]

                this is almost the same as used in v3D::Core::Camera::createProjection()
                the only difference is that tx and ty are negated.
            */

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

            float left = -1.0;
            float right = 1.0;  // xres_;
            float top = 1.0;
            float bottom = -1.0;  // yres_;
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
            projection[2][2] = -2.0f / (far - near);
            projection[2][2] = -tz;
            projection[0][3] = 0.0;
            projection[1][3] = 0.0;
            projection[2][3] = 0.0;
            projection[3][3] = 1.0;

            // std::cerr << projection;
        } else {  // implementation-specific projection
            // unsupported projections default to orthographic
        }

        // set transform to screen transformation


        // append projection matrix to current transformation matrix
        transform_ *= projection;
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
        transforms_.pop_back();
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

    glm::mat4x4 RenderContext::coordinateSystem(const std::string& name) {
        return coordinateSystems_[name];
    }

    void RenderContext::translate(float dx, float dy, float dz) {
        transform_ = glm::translate(transform_, glm::vec3(dx, dy, dz));
    }

    void RenderContext::rotate(float angle, float dx, float dy, float dz) {
    }

    void RenderContext::scale(float sx, float sy, float sz) {
    }

    /*
        maps to RiPolygon()
        this covers the initial pass of the reyes architecture
    */
    void RenderContext::addPolygon(boost::shared_ptr<Polygon> poly) {
        // if an output stream exists
        // echo RiPolygon RIB command to output stream

        // _polygons.push_back(poly);

        // bound polygon in eye space
        /*
            if this is just based off of poly vertices, the bound will be in object space
            we'll need to apply the current modeling transformation to get from object space to world space
            and then the camera transformation will need to be applied to get into eye space
            we should probably just transform the poly to eye space first since any future calculations
            on this poly will be done in eye space or beyond.
        */
        v3d::type::AABBox bound = poly->bound();

        glm::vec3 bound_max = bound.max();
        glm::vec3 bound_min = bound.min();

        /*
            convert bound to eye space coordinates
            first multiply by modeling transformation to get world coordinates
            next multiply by camera transformation to get camera/eye coordinates

            for now we're just taking the current transform.
            later we'll probably need to concatenate the transforms_ matrix stack too
        */

        bound_max = glm::vec3(transform_ * glm::vec4(bound_max, 1.0f));
        bound_max = glm::vec3(glm::transpose(coordinateSystems_["camera"]) * glm::vec4(bound_max, 1.0f));
        bound_min = glm::vec3(transform_ * glm::vec4(bound_min, 1.0f));
        bound_min = glm::vec3(glm::transpose(coordinateSystems_["camera"]) * glm::vec4(bound_min, 1.0f));

        // camera transform might've flipped some components of min & max
        if (bound_min[0] > bound_max[0])  {
            std::swap(bound_min[0], bound_max[0]);
        }
        if (bound_min[1] > bound_max[1]) {
            std::swap(bound_min[1], bound_max[1]);
        }
        if (bound_min[2] > bound_max[2]) {
            std::swap(bound_min[2], bound_max[2]);
        }

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

        // bound = poly->bound();
        glm::mat4x4 screen = coordinateSystems_["screen"];
        screen *= coordinateSystems_["raster"];
        bound_min = glm::vec3(screen * glm::vec4(bound_min, 1.0f));
        bound_max = glm::vec3(screen * glm::vec4(bound_max, 1.0f));

        // screen transform might've flipped some components of min & max
        if (bound_min[0] > bound_max[0])
            std::swap(bound_min[0], bound_max[0]);
        if (bound_min[1] > bound_max[1])
            std::swap(bound_min[1], bound_max[1]);
        if (bound_min[2] > bound_max[2])
            std::swap(bound_min[2], bound_max[2]);

        bound.extents(bound_min, bound_max);

        // do viewing frustum cull
        Frustum frustum(screen);
        if (frustum.intersect(bound) == Frustum::OUTSIDE) {  // poly is entirely outside frustum
            return;
        }

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
            for (unsigned int i = 0; i < poly->vertexCount(); i++) {
                glm::mat4x4 em = transform_ * glm::transpose(coordinateSystems_["camera"]);
                Vertex pv = poly->vertex(i);
                pv.point(glm::vec3(em * glm::vec4(pv.point(), 1.0f)));
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
    unsigned int RenderContext::getPolygonCount(void) const
    {
      return _polygons.size();
    }

    PolygonPtr RenderContext::getPolygon(unsigned int idx) const
    {
      assert(idx >= 0 && idx < _polygons.size());
      return _polygons[idx];
    }
    */


    /*
        maps to RiWorldEnd()
        once rendering is done, objects, lights and other stuff set
        after prepareWorld() are destroyed and the memory reclaimed
        saving the output file is done here also.
        the results might not even include a rendered image depending
        on the context. we might just output a rib file.
    */
    /*
        perform the second reyes pass
    */
    void RenderContext::render() {
        frameBuffer_->render();
    }

};  // namespace v3d::moya
