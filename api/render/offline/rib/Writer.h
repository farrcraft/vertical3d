/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Sampling.h>

#include <iosfwd>
#include <string>
#include <vector>

#include "Declarations.h"
#include "Handler.h"
#include "Parameters.h"
#include "Requests.h"

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
    void declare(const Declare & request) override;
    void option(const std::string & name, const ParameterList & parameters) override;
    void hider(const std::string & name, const ParameterList & parameters) override;

    void format(const Format & request) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(const ScreenWindow & request) override;
    void cropWindow(const CropWindow & request) override;
    void projection(const std::string & name, const ParameterList & parameters) override;
    void clipping(const Clipping & request) override;
    void depthOfField(const DepthOfField & request) override;
    void shutter(const Shutter & request) override;

    void pixelSamples(const glm::uvec2 & samples) override;
    void pixelFilter(Filter filter, float xwidth, float ywidth) override;
    void pixelVariance(float variation) override;
    void display(const Display & request, const ParameterList & parameters) override;

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
    void basis(const Basis & request) override;
    void opacity(const glm::vec3 & value) override;
    void shadingRate(float size) override;
    void attribute(const std::string & name, const ParameterList & parameters) override;
    void surface(const std::string & name, const ParameterList & parameters) override;
    void displacement(const std::string & name, const ParameterList & parameters) override;
    void lightSource(const LightSource & request, const ParameterList & parameters) override;
    void areaLightSource(const LightSource & request, const ParameterList & parameters) override;
    void illuminate(const std::string & handle, bool on) override;
    void makeTexture(const MakeTexture & request, const ParameterList & parameters) override;
    void imager(const std::string & name, const ParameterList & parameters) override;

    Handler* deformation() override;
    void polygon(unsigned int vertices, const ParameterList & parameters) override;
    void pointsPolygons(const PointsPolygons & request, const ParameterList & parameters) override;
    void sphere(const Sphere & request, const ParameterList & parameters) override;
    void curves(const Curves & request, const ParameterList & parameters) override;

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
    /** A light handle: a number when it is one, as RIB 3.03 writes it, and a string otherwise. **/
    Writer & handle(const std::string & value);
    /** Each parameter's name and its values, after a space. **/
    Writer & parameters(const ParameterList & list);
    /** End the request's line. **/
    void end();

    std::ostream * stream_;
    Declarations declarations_;
};

};  // namespace v3d::render::offline::rib
