/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/requests/Basis.h>
#include <api/render/offline/rib/requests/Clipping.h>
#include <api/render/offline/rib/requests/Cone.h>
#include <api/render/offline/rib/requests/CropWindow.h>
#include <api/render/offline/rib/requests/Curves.h>
#include <api/render/offline/rib/requests/Cylinder.h>
#include <api/render/offline/rib/requests/Declare.h>
#include <api/render/offline/rib/requests/DepthOfField.h>
#include <api/render/offline/rib/requests/Disk.h>
#include <api/render/offline/rib/requests/Display.h>
#include <api/render/offline/rib/requests/DisplayChannel.h>
#include <api/render/offline/rib/requests/Format.h>
#include <api/render/offline/rib/requests/Hyperboloid.h>
#include <api/render/offline/rib/requests/LightSource.h>
#include <api/render/offline/rib/requests/MakeTexture.h>
#include <api/render/offline/rib/requests/Paraboloid.h>
#include <api/render/offline/rib/requests/PointsPolygons.h>
#include <api/render/offline/rib/requests/ScreenWindow.h>
#include <api/render/offline/rib/requests/ShadingNode.h>
#include <api/render/offline/rib/requests/Shutter.h>
#include <api/render/offline/rib/requests/Sphere.h>
#include <api/render/offline/rib/requests/SubdivisionMesh.h>
#include <api/render/offline/rib/requests/Torus.h>

#include <string>
#include <vector>

#include "Parameters.h"

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
    virtual void declare(const requests::Declare & request) { (void)request; }
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
    virtual void format(const requests::Format & request) { (void)request; }
    virtual void frameAspectRatio(float aspect) { (void)aspect; }
    virtual void screenWindow(const requests::ScreenWindow & request) { (void)request; }
    virtual void cropWindow(const requests::CropWindow & request) { (void)request; }
    virtual void projection(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }
    virtual void clipping(const requests::Clipping & request) { (void)request; }
    /**
     * RIB's DepthOfField with no arguments sets a pinhole, which arrives as an infinite fstop.
     **/
    virtual void depthOfField(const requests::DepthOfField & request) { (void)request; }
    virtual void shutter(const requests::Shutter & request) { (void)request; }

    // how a pixel is sampled
    virtual void pixelSamples(const glm::uvec2 & samples) { (void)samples; }
    virtual void pixelFilter(film::Filter filter, float xwidth, float ywidth) {
        (void)filter;
        (void)xwidth;
        (void)ywidth;
    }
    virtual void pixelVariance(float variation) { (void)variation; }
    virtual void display(const requests::Display & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * RenderMan's `DisplayChannel`: a channel a later Display's mode can name, and what it
     * collects. RI has no such request, so the default body ignores it.
     **/
    virtual void displayChannel(const requests::DisplayChannel & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * RenderMan's `SampleFilter`: a filter of a type and a handle that each camera sample passes
     * through, such as `PxrCryptomatte`. RI has no such request, so the default body ignores it.
     **/
    virtual void sampleFilter(const requests::ShadingNode & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * RenderMan's `DisplayFilter`: a filter of a type and a handle that the finished pixels pass
     * through before they are written, such as `PxrBackgroundDisplayFilter`. RI has no such
     * request, so the default body ignores it.
     **/
    virtual void displayFilter(const requests::ShadingNode & request, const ParameterList & parameters) {
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
     * @param axis the direction to turn about, through the origin of the current space. It
     *        need not be unit length
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
     * RiCoordinateSystem: give the current transformation a name that a later request or a
     * shader can refer to.
     **/
    virtual void coordinateSystem(const std::string & space) { (void)space; }
    /**
     * RiCoordSysTransform: make a named space the current transformation.
     **/
    virtual void coordSysTransform(const std::string & space) { (void)space; }
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
    virtual void basis(const requests::Basis & request) { (void)request; }
    virtual void surface(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }
    virtual void displacement(const std::string & name, const ParameterList & parameters) {
        (void)name;
        (void)parameters;
    }
    /**
     * RenderMan's `Pattern`: a node of a shading network, whose outputs a later node's reference
     * parameters read. RI's shading language has no such node, so the default body ignores it.
     **/
    virtual void pattern(const requests::ShadingNode & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * RenderMan's `Bxdf`: the node of a shading network that gives a surface's scattering. The
     * default body ignores it, as it does a Pattern.
     **/
    virtual void bxdf(const requests::ShadingNode & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void lightSource(const requests::LightSource & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * RenderMan's `Light`, which names a light such as `PxrRectLight` and its handle. RI's
     * shading language lights do not include these, so the default body ignores the request.
     **/
    virtual void light(const requests::LightSource & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * A light whose shape matters, bound to the geometry that follows it. A renderer that
     * cannot sample a light's area treats it as an ordinary light. The default body does
     * this, so a scene that uses one is still lit.
     **/
    virtual void areaLightSource(const requests::LightSource & request, const ParameterList & parameters) {
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
    virtual void makeTexture(const requests::MakeTexture & request, const ParameterList & parameters) {
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
    virtual void pointsPolygons(const requests::PointsPolygons & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void sphere(const requests::Sphere & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void curves(const requests::Curves & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    /**
     * Points, each a small sphere or disk. RIB carries no count. The reader takes it from the
     * length of "P".
     **/
    virtual void points(unsigned int count, const ParameterList & parameters) {
        (void)count;
        (void)parameters;
    }
    virtual void cone(const requests::Cone & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void cylinder(const requests::Cylinder & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void hyperboloid(const requests::Hyperboloid & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void paraboloid(const requests::Paraboloid & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void disk(const requests::Disk & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void torus(const requests::Torus & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }
    virtual void subdivisionMesh(const requests::SubdivisionMesh & request, const ParameterList & parameters) {
        (void)request;
        (void)parameters;
    }

    // instancing
    /**
     * RiObjectBegin: the geometry up to objectEnd() is an object, held to be instanced rather than
     * drawn. RIB names it by a number or a string. An object holds geometry alone. Each instance
     * takes the attributes and transformation in force where it is placed.
     **/
    virtual void objectBegin(const std::string & handle) { (void)handle; }
    virtual void objectEnd() { }
    /** RiObjectInstance: place an object, which an objectBegin() before it named. **/
    virtual void objectInstance(const std::string & handle) { (void)handle; }
};

};  // namespace v3d::render::offline::rib
