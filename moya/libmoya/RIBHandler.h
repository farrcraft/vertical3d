/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Handler.h>

#include <string>
#include <vector>

#include "RenderContext.h"

namespace v3d::moya {

/**
 * Drives a render context from RI requests. The RIB reader calls one typed method per request,
 * and so do the C entry points in RenderMan.cxx, so each RI rule this renderer follows is
 * written here once.
 *
 * A handler owns its render context and the declarations its parameters are typed by. RiBegin
 * and RiEnd have no RIB equivalent: the standard implies them at the start and end of a file,
 * and a handler's life is that span.
 *
 * A request whose shader, light or space name is empty names nothing, and is ignored. The C
 * interface passes RI_NULL on as the empty name.
 *
 * Moya is not a RenderMan compliant renderer. Compliance forbids any way into the renderer
 * besides the RI calls and RIB, and RenderContext is another way in. A program that uses the
 * RI standard without being compliant must display Pixar's copyright notice. The first handler
 * a process makes prints it on stdout.
 **/
class RIBHandler final : public v3d::render::offline::rib::Handler {
 public:
    /**
     * @param name stored with the context and not otherwise read. An empty string is RI_NULL
     **/
    explicit RIBHandler(const std::string & name = "");
    RIBHandler(const RIBHandler &) = delete;
    RIBHandler & operator=(const RIBHandler &) = delete;

    /**
     * Records the declaration for the parameters the C interface passes. The RIB reader keeps
     * its own for the file it reads, and this one as well.
     **/
    void declare(const v3d::render::offline::rib::Declare & request) override;

    void option(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void hider(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;

    void format(const v3d::render::offline::rib::Format & request) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(const v3d::render::offline::rib::ScreenWindow & request) override;
    void cropWindow(const v3d::render::offline::rib::CropWindow & request) override;
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
    void coordinateSystem(const std::string & space) override;
    void coordSysTransform(const std::string & space) override;
    /**
     * Each primitive after the first inside the block is the same primitive at a later time.
     * moya does not deform a primitive, so it draws the first one only, and reports the first
     * one it leaves out. The RIB reader holds such a primitive back from a file itself.
     **/
    void motionBegin(const std::vector<float> & times) override;
    void motionEnd() override;

    void color(const glm::vec3 & value) override;
    void opacity(const glm::vec3 & value) override;
    void shadingRate(float size) override;
    void basis(const v3d::render::offline::rib::Basis & request) override;

    void surface(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void lightSource(const v3d::render::offline::rib::LightSource & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void displacement(const std::string & name,
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
    void curves(const v3d::render::offline::rib::Curves & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    /**
     * Drawn by the ray hider only, because the reyes hider does not dice spheres. The first
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
     * What the scene has declared, and what the standard declares for it. The C interface
     * types the parameters it is passed by these.
     **/
    const v3d::render::offline::rib::Declarations & declarations() const;

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
    /** Whether the primitive arriving now deforms one already in the motion block. **/
    bool deforms();

    RenderContext context_;
    v3d::render::offline::rib::Declarations declarations_;
    std::string output_;
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    bool spheres_ = false;
    bool motion_ = false;
    unsigned int primitives_ = 0;
    bool deformed_ = false;
};

};  // namespace v3d::moya
