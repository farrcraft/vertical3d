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
#include <api/render/offline/rib/requests/ShadingNode.h>
#include <api/render/offline/rib/requests/MakeTexture.h>
#include <api/render/offline/rib/requests/Paraboloid.h>
#include <api/render/offline/rib/requests/PointsPolygons.h>
#include <api/render/offline/rib/requests/ScreenWindow.h>
#include <api/render/offline/rib/requests/Shutter.h>
#include <api/render/offline/rib/requests/Sphere.h>
#include <api/render/offline/rib/requests/SubdivisionMesh.h>
#include <api/render/offline/rib/requests/Torus.h>

#include <iosfwd>
#include <string>
#include <vector>

#include "Declarations.h"
#include "Handler.h"
#include "Parameters.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

/**
 * Writes each request it receives as a line of ASCII RIB, which Reader reads back as the same
 * request.
 *
 * A number is written in the fewest digits that read back as the same float. RIB has no
 * number for an infinity or a NaN, so an infinity is written as the largest float of its sign
 * and a NaN as 0. DepthOfField's infinite fstop is the exception: it is written as the request
 * with no arguments, which RIB reads as a pinhole.
 *
 * A parameter is written by its name alone when its declaration is the one in force, from the
 * standard or an earlier declare(). Any other parameter carries its declaration inline, as in
 * "uniform float Ks".
 *
 * A primitive after the first in a motion block is written as it is, because RIB carries a
 * deformation as the same primitive given again.
 **/
class Writer final : public Handler {
 public:
    /**
     * @param stream where the RIB goes. Nothing here closes or flushes it
     **/
    explicit Writer(std::ostream * stream);

    void version(float number) override;
    void declare(const requests::Declare & request) override;
    void option(const std::string & name, const ParameterList & parameters) override;
    void hider(const std::string & name, const ParameterList & parameters) override;

    void format(const requests::Format & request) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(const requests::ScreenWindow & request) override;
    void cropWindow(const requests::CropWindow & request) override;
    void projection(const std::string & name, const ParameterList & parameters) override;
    void clipping(const requests::Clipping & request) override;
    void depthOfField(const requests::DepthOfField & request) override;
    void shutter(const requests::Shutter & request) override;

    void pixelSamples(const glm::uvec2 & samples) override;
    void pixelFilter(film::Filter filter, float xwidth, float ywidth) override;
    void pixelVariance(float variation) override;
    void display(const requests::Display & request, const ParameterList & parameters) override;
    void displayChannel(const requests::DisplayChannel & request, const ParameterList & parameters) override;
    void sampleFilter(const requests::ShadingNode & request, const ParameterList & parameters) override;
    void displayFilter(const requests::ShadingNode & request, const ParameterList & parameters) override;

    void frameBegin(int frame) override;
    void frameEnd() override;
    void worldBegin() override;
    void worldEnd() override;
    void attributeBegin() override;
    void attributeEnd() override;
    void transformBegin() override;
    void transformEnd() override;

    void identity() override;
    void transform(const glm::mat4x4 & matrix) override;  // NOLINT(build/include_what_you_use)
    void concatTransform(const glm::mat4x4 & matrix) override;
    void translate(float dx, float dy, float dz) override;
    void rotate(float angle, const glm::vec3 & axis) override;
    void scale(float sx, float sy, float sz) override;
    void coordinateSystem(const std::string & space) override;
    void coordSysTransform(const std::string & space) override;
    void motionBegin(const std::vector<float> & times) override;
    void motionEnd() override;

    void color(const glm::vec3 & value) override;
    void basis(const requests::Basis & request) override;
    void opacity(const glm::vec3 & value) override;
    void shadingRate(float size) override;
    void attribute(const std::string & name, const ParameterList & parameters) override;
    void surface(const std::string & name, const ParameterList & parameters) override;
    void displacement(const std::string & name, const ParameterList & parameters) override;
    void pattern(const requests::ShadingNode & request, const ParameterList & parameters) override;
    void bxdf(const requests::ShadingNode & request, const ParameterList & parameters) override;
    void lightSource(const requests::LightSource & request, const ParameterList & parameters) override;
    void light(const requests::LightSource & request, const ParameterList & parameters) override;
    void areaLightSource(const requests::LightSource & request, const ParameterList & parameters) override;
    void illuminate(const std::string & handle, bool on) override;
    void makeTexture(const requests::MakeTexture & request, const ParameterList & parameters) override;
    void imager(const std::string & name, const ParameterList & parameters) override;

    Handler* deformation() override;
    void polygon(unsigned int vertices, const ParameterList & parameters) override;
    void pointsPolygons(const requests::PointsPolygons & request, const ParameterList & parameters) override;
    void sphere(const requests::Sphere & request, const ParameterList & parameters) override;
    void curves(const requests::Curves & request, const ParameterList & parameters) override;
    void points(unsigned int count, const ParameterList & parameters) override;
    void cone(const requests::Cone & request, const ParameterList & parameters) override;
    void cylinder(const requests::Cylinder & request, const ParameterList & parameters) override;
    void hyperboloid(const requests::Hyperboloid & request, const ParameterList & parameters) override;
    void paraboloid(const requests::Paraboloid & request, const ParameterList & parameters) override;
    void disk(const requests::Disk & request, const ParameterList & parameters) override;
    void torus(const requests::Torus & request, const ParameterList & parameters) override;
    void subdivisionMesh(const requests::SubdivisionMesh & request, const ParameterList & parameters) override;

    void objectBegin(const std::string & handle) override;
    void objectEnd() override;
    void objectInstance(const std::string & handle) override;

 private:
    /** A request's name, which starts its line. **/
    Writer & start(const char * name);
    /** A number, after a space. **/
    Writer & number(float value);
    /** A string in quotes, after a space, with a quote, a backslash and a newline escaped. **/
    Writer & text(const std::string & value);
    /** Numbers in brackets, after a space. **/
    Writer & array(const std::vector<float> & values);
    /** Counts in brackets, after a space, written whole. **/
    Writer & counts(const std::vector<unsigned int> & values);
    /** Strings in brackets, after a space, each as text() writes it. **/
    Writer & strings(const std::vector<std::string> & values);
    /**
     * A light's or an object's handle: a number when it is one, as RIB 3.03 writes it, and a
     * string otherwise.
     **/
    Writer & handle(const std::string & value);
    /** Each parameter's name and its values, after a space. **/
    Writer & parameters(const ParameterList & list);
    /** End the request's line. **/
    void end();

    std::ostream * stream_;
    Declarations declarations_;
};

};  // namespace v3d::render::offline::rib
