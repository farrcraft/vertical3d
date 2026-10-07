/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Sampling.h>

#include <string>
#include <vector>

#include "Parameters.h"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

/**
 * The interface a RIB reader calls on a renderer, one method per request.
 *
 * The methods mirror the RI request set. A method that does not correspond to an RI request
 * does not belong here.
 *
 * **Every method has an empty body rather than being pure virtual.** The RI standard requires
 * a renderer to accept a request for a feature it does not support, and a request added
 * later then breaks no handler. A misspelled override is silent as a result, so every
 * override carries `override`.
 **/
class Handler {
 public:
    virtual ~Handler() { }

    virtual void version(float number) { (void)number; }
    virtual void declare(const std::string & name, const std::string & declaration) {
        (void)name;
        (void)declaration;
    }
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
    /**
     * RiFormat. RI reads a side of zero or less as the renderer's default for that side, and
     * such a side arrives as 0. Every other side is from 1 to largestResolution.
     **/
    virtual void format(unsigned int width, unsigned int height, float pixelAspect) {
        (void)width;
        (void)height;
        (void)pixelAspect;
    }
    virtual void frameAspectRatio(float aspect) { (void)aspect; }
    virtual void screenWindow(float left, float right, float bottom, float top) {
        (void)left;
        (void)right;
        (void)bottom;
        (void)top;
    }
    virtual void cropWindow(float xmin, float xmax, float ymin, float ymax) {
        (void)xmin;
        (void)xmax;
        (void)ymin;
        (void)ymax;
    }
    virtual void projection(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }
    virtual void clipping(float hither, float yon) {
        (void)hither;
        (void)yon;
    }
    /**
     * @param fstop an infinite one is a pinhole; RIB's DepthOfField with no arguments also
     *        sets a pinhole
     **/
    virtual void depthOfField(float fstop, float focalLength, float focalDistance) {
        (void)fstop;
        (void)focalLength;
        (void)focalDistance;
    }
    virtual void shutter(float open, float close) {
        (void)open;
        (void)close;
    }

    // how a pixel is sampled
    virtual void pixelSamples(unsigned int x, unsigned int y) {
        (void)x;
        (void)y;
    }
    virtual void pixelFilter(Filter filter, float xwidth, float ywidth) {
        (void)filter;
        (void)xwidth;
        (void)ywidth;
    }
    virtual void pixelVariance(float variation) { (void)variation; }
    virtual void display(const std::string & name, const std::string & type, const std::string & mode,
        const ParameterList & parameters) {
        (void)name;
        (void)type;
        (void)mode;
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
    virtual void rotate(float angle, float dx, float dy, float dz) {
        (void)angle;
        (void)dx;
        (void)dy;
        (void)dz;
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
    /**
     * @param handle what a later Illuminate names this light by
     *
     * RIB 3.03 writes the handle as a sequence number and later RIB writes a string. Both
     * are read and it is a string here either way, so a renderer keying a map on it does
     * not need to handle both forms.
     **/
    virtual void lightSource(const std::string & name, const std::string & handle,
        const ParameterList & parameters) {
        (void)name;
        (void)handle;
        (void)parameters;
    }
    /**
     * A light whose shape matters, bound to the geometry that follows it. A renderer that
     * cannot sample a light's area treats it as an ordinary light. The default body does
     * this, so a scene that uses one is still lit.
     **/
    virtual void areaLightSource(const std::string & name, const std::string & handle,
        const ParameterList & parameters) {
        lightSource(name, handle, parameters);
    }
    /**
     * Turn a light on or off in the current attribute state.
     **/
    virtual void illuminate(const std::string & handle, bool on) {
        (void)handle;
        (void)on;
    }
    /**
     * Make a texture file from an image. Nothing by default: a renderer that reads the image
     * a scene names as the texture has nothing to make, and the request is still understood
     * rather than unrecognised.
     *
     * @param filter the name of the filter the texture is made with
     **/
    virtual void makeTexture(const std::string & picture, const std::string & texture, const std::string & swrap,
        const std::string & twrap, const std::string & filter, float swidth, float twidth,
        const ParameterList & parameters) {
        (void)picture;
        (void)texture;
        (void)swrap;
        (void)twrap;
        (void)filter;
        (void)swidth;
        (void)twidth;
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
     * primitive is the same one at a later time, so it deforms. Null by default: the reader
     * then reads the primitive to keep the stream in step, drops it, and lists it as
     * unsupported, so the primitive is drawn at the block's first time.
     **/
    virtual Handler* deformation() { return nullptr; }

    /**
     * One closed planar convex polygon. RIB carries no vertex count - it is the length of
     * the "P" array, which the reader has already divided out.
     **/
    virtual void polygon(unsigned int vertices, const ParameterList & parameters) {
        (void)vertices;
        (void)parameters;
    }
    /**
     * @param counts how many vertices each polygon has
     * @param indices the vertices of every polygon in turn, indexing the "P" array
     **/
    virtual void pointsPolygons(const std::vector<unsigned int> & counts,
        const std::vector<unsigned int> & indices, const ParameterList & parameters) {
        (void)counts;
        (void)indices;
        (void)parameters;
    }
    virtual void sphere(float radius, float zmin, float zmax, float thetamax, const ParameterList & parameters) {
        (void)radius;
        (void)zmin;
        (void)zmax;
        (void)thetamax;
        (void)parameters;
    }
};

};  // namespace v3d::render::offline::rib
