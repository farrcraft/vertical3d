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

#include <functional>
#include <string>
#include <vector>

#include "Handler.h"
#include "Parameters.h"

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
    void sides(int count) override;
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
    typedef std::function<void(Handler &)> Request;

    /**
     * The deformations of a recording: each primitive it receives is kept in the recording,
     * and replayed to the deformation() of the handler the recording replays to.
     **/
    class Deforming final : public Handler {
     public:
        explicit Deforming(Recorder * recording);
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

     private:
        /** Keep a primitive, for the deformation() of the handler replayed to. **/
        void keep(const Request & primitive);

        Recorder * recording_;
    };

    std::vector<Request> requests_;
    Deforming deforming_{ this };
};

};  // namespace v3d::render::offline::rib
