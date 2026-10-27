/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/Sampling.h>

#include <functional>
#include <string>
#include <vector>

#include "Handler.h"
#include "Parameters.h"
#include "Requests.h"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::render::offline::rib {

/**
 * Keeps each request it receives, with copies of its arguments, and passes them in the same
 * order to another handler later.
 *
 * A scene can be gathered where it is safe to read, such as on the thread that edits it, and
 * sent to a renderer on another thread. A recording holds no reference to anything it was
 * given, so it outlives the scene.
 *
 * A recording replays a deforming primitive to the handler's deformation(), as a reader would.
 **/
class Recorder final : public Handler {
 public:
    Recorder() = default;
    Recorder(const Recorder &) = delete;
    Recorder & operator=(const Recorder &) = delete;
    Recorder(Recorder &&) = delete;
    Recorder & operator=(Recorder &&) = delete;
    ~Recorder() override = default;

    /**
     * Pass every request kept so far to a handler, in the order they arrived. The recording
     * is unchanged, so it can be replayed again.
     **/
    void replay(Handler * handler) const;

    /** Whether no request has been kept. **/
    bool empty() const;

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
    typedef std::function<void(Handler &)> Request;

    /**
     * The deformations of a recording: each primitive it receives is kept in the recording,
     * and replayed to the deformation() of the handler the recording replays to.
     **/
    class Deforming final : public Handler {
     public:
        explicit Deforming(Recorder * recording);
        void polygon(unsigned int vertices, const ParameterList & parameters) override;
        void pointsPolygons(const PointsPolygons & request, const ParameterList & parameters) override;
        void sphere(const Sphere & request, const ParameterList & parameters) override;
        void curves(const Curves & request, const ParameterList & parameters) override;

     private:
        /** Keep a primitive, for the deformation() of the handler replayed to. **/
        void keep(const Request & primitive);

        Recorder * recording_;
    };

    std::vector<Request> requests_;
    Deforming deforming_{ this };
};

};  // namespace v3d::render::offline::rib
