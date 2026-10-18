/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Sampling.h>

#include <string>
#include <vector>

#include "Parameters.h"
#include "Requests.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

/**
 * The interface a RIB reader calls on a renderer, one method per request.
 *
 * The methods mirror the RI request set. A method that does not correspond to an RI request
 * does not belong here.
 *
 * **Every method has an empty body, not a pure virtual one.** The RI standard requires a
 * renderer to accept a request for a feature it does not support. A method added here also
 * breaks no existing handler. A misspelled override would compile as a new method, so every
 * override carries `override`.
 **/
class Handler {
 public:
    virtual ~Handler() { }

    virtual void version(float number) { (void)number; }
    virtual void declare(const Declare & request) { (void)request; }
    virtual void option(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }
    /**
     * RiHider: how the renderer decides what the camera sees. RI names `"hidden"` as the
     * default; any other name is the renderer's own.
     **/
    virtual void hider(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }

    // the camera
    virtual void format(const Format & request) { (void)request; }
    virtual void frameAspectRatio(float aspect) { (void)aspect; }
    virtual void screenWindow(const ScreenWindow & request) { (void)request; }
    virtual void cropWindow(const CropWindow & request) { (void)request; }
    virtual void projection(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }
    virtual void clipping(const Clipping & request) { (void)request; }
    /**
     * RIB's DepthOfField with no arguments sets a pinhole, which arrives as an infinite fstop.
     **/
    virtual void depthOfField(const DepthOfField & request) { (void)request; }
    virtual void shutter(const Shutter & request) { (void)request; }

    // how a pixel is sampled
    virtual void pixelSamples(const glm::uvec2 & samples) { (void)samples; }
    virtual void pixelFilter(Filter filter, float xwidth, float ywidth) {
        (void)filter;
        (void)xwidth;
        (void)ywidth;
    }
    virtual void pixelVariance(float variation) { (void)variation; }
    virtual void display(const Display & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }

    // block structure
    virtual void frameBegin(int frame) { (void)frame; }
    virtual void frameEnd() { }
    virtual void worldBegin() { }
    virtual void worldEnd() { }
    virtual void attributeBegin() { }
    virtual void attributeEnd() { }
    virtual void transformBegin() { }
    virtual void transformEnd() { }

    // the transformation stack
    virtual void identity() { }
    virtual void transform(const glm::mat4x4 & matrix) { (void)matrix; }  // NOLINT(build/include_what_you_use) - RiTransform, not std::transform
    virtual void concatTransform(const glm::mat4x4 & matrix) { (void)matrix; }
    virtual void translate(float dx, float dy, float dz) {
        (void)dx;
        (void)dy;
        (void)dz;
    }
    /**
     * @param angle in degrees, counter clockwise about the axis
     **/
    virtual void rotate(float angle, const glm::vec3 & axis) {
        (void)angle;
        (void)axis;
    }
    virtual void scale(float sx, float sy, float sz) {
        (void)sx;
        (void)sy;
        (void)sz;
    }
    /**
     * Each transform request up to motionEnd() is the transformation at the next of these
     * times. A primitive inside the block reaches the handler once, at the first time. Each
     * later copy of it goes to deformation().
     **/
    virtual void motionBegin(const std::vector<float> & times) { (void)times; }
    virtual void motionEnd() { }

    // the graphics state
    virtual void color(const glm::vec3 & value) { (void)value; }
    virtual void opacity(const glm::vec3 & value) { (void)value; }
    virtual void shadingRate(float size) { (void)size; }
    virtual void attribute(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }
    virtual void surface(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }
    virtual void lightSource(const LightSource & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * A light whose shape matters, bound to the geometry that follows it. A renderer that
     * cannot sample a light's area treats it as an ordinary light. The default body does
     * this, so a scene that uses one is still lit.
     **/
    virtual void areaLightSource(const LightSource & request, const ParameterList & parameters) {
        lightSource(request, parameters);
    }
    /**
     * Turn a light on or off in the current attribute state.
     **/
    virtual void illuminate(const std::string & handle, bool on) {
        (void)handle;
        (void)on;
    }
    /**
     * Make a texture file from an image. Does nothing by default. A renderer that reads the
     * named image directly as the texture has nothing to make. The reader still counts the
     * request as recognised.
     **/
    virtual void makeTexture(const MakeTexture & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * The shader run over the finished framebuffer. A scene uses it to set the value of a
     * pixel that nothing was drawn into.
     **/
    virtual void imager(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }

    // geometry
    /**
     * The handler that receives each primitive after the first in a motion block. Such a
     * primitive is the first one at a later time, so the primitive deforms. Null by default.
     * The reader then still parses the primitive, discards it, and lists it as unsupported.
     * The primitive is drawn at the block's first time only.
     **/
    virtual Handler* deformation() { return nullptr; }

    /**
     * One closed planar convex polygon. RIB carries no vertex count. The reader takes it
     * from the length of "P", or of "Pw" or "Pz" when there is no "P".
     **/
    virtual void polygon(unsigned int vertices, const ParameterList & parameters) {
        (void)vertices;
        (void)parameters;
    }
    virtual void pointsPolygons(const PointsPolygons & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void sphere(const Sphere & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
};

};  // namespace v3d::render::offline::rib
