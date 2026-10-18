/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/rib/Handler.h>

#include <string>
#include <vector>

#include "Renderer.h"

namespace v3d::moya {

/**
 * Drives a render context from a RIB stream. The reader calls one typed method per RI request.
 *
 * RiBegin and RiEnd have no RIB equivalent. The standard implies them at the start and end
 * of a file, so the constructor pushes a render context and the destructor pops it.
 *
 * The C entry points in RenderMan.cxx are the other way into the same render context.
 * Neither calls the other. The reader passes typed parameter lists, and the C entry points
 * take untyped arrays.
 **/
class RIBHandler final : public v3d::render::offline::rib::Handler {
 public:
    explicit RIBHandler(Renderer * renderer);
    ~RIBHandler();

    void option(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void hider(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;

    void format(const v3d::render::offline::rib::Format & request) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(const v3d::render::offline::rib::ScreenWindow & request) override;
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void clipping(const v3d::render::offline::rib::Clipping & request) override;
    void depthOfField(const v3d::render::offline::rib::DepthOfField & request) override;
    void shutter(const v3d::render::offline::rib::Shutter & request) override;
    void pixelSamples(const glm::uvec2 & samples) override;
    void pixelFilter(v3d::render::offline::Filter filter, float xwidth, float ywidth) override;
    void pixelVariance(float variation) override;
    void display(const v3d::render::offline::rib::Display & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;

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
    void motionBegin(const std::vector<float> & times) override;
    void motionEnd() override;

    void color(const glm::vec3 & value) override;
    void opacity(const glm::vec3 & value) override;
    void shadingRate(float size) override;

    void surface(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void lightSource(const v3d::render::offline::rib::LightSource & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void attribute(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void areaLightSource(const v3d::render::offline::rib::LightSource & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void illuminate(const std::string & handle, bool on) override;
    void imager(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;

    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override;
    void pointsPolygons(const v3d::render::offline::rib::PointsPolygons & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    /**
     * Drawn by the ray hider only, because the reyes hider dices polygons only. The first
     * sphere a scene names under the reyes hider is reported.
     **/
    void sphere(const v3d::render::offline::rib::Sphere & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;

    /**
     * The context the requests reach. Its framebuffer holds the picture once the file has
     * been read.
     **/
    RenderContext & context();

    /**
     * The file the render is written to, whatever the scene's Display names. It is set
     * before the scene is read, and a Display the scene makes later keeps this name.
     **/
    void output(const std::string & name);

    /**
     * The size of the picture whatever the scene's Format says. A scene's Format still sets
     * the pixel aspect ratio, and arrives after this is set.
     *
     * @return false, changing nothing, when a side is below 1 or above largestResolution
     **/
    bool resolution(int width, int height);

    /** The largest grid or bucket size, in micropolygons or pixels, that is not refused. **/
    static constexpr unsigned int largestLimit = 65536;

 private:
    Renderer * renderer_;
    std::string output_;
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    bool spheres_ = false;
};

};  // namespace v3d::moya
