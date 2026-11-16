/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/render/offline/rib/Declarations.h>
#include <api/render/offline/rib/Handler.h>
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
#include <api/render/offline/rib/requests/Format.h>
#include <api/render/offline/rib/requests/Hyperboloid.h>
#include <api/render/offline/rib/requests/LightSource.h>
#include <api/render/offline/rib/requests/ShadingNode.h>
#include <api/render/offline/rib/requests/Paraboloid.h>
#include <api/render/offline/rib/requests/PointsPolygons.h>
#include <api/render/offline/rib/requests/ScreenWindow.h>
#include <api/render/offline/rib/requests/Shutter.h>
#include <api/render/offline/rib/requests/Sphere.h>
#include <api/render/offline/rib/requests/SubdivisionMesh.h>
#include <api/render/offline/rib/requests/Torus.h>
#include <moya/libmoya/state/RenderContext.h>

#include <cstdint>
#include <optional>
#include <set>
#include <string>
#include <vector>

namespace v3d::moya::ri {

/**
 * Drives a render context from RI requests. The RIB reader calls one typed method per request.
 *
 * A handler owns its render context and the declarations its parameters are typed by. RiBegin
 * and RiEnd have no RIB equivalent: the standard implies them at the start and end of a file,
 * and a handler's life is that span.
 *
 * A request whose shader, light or space name is empty names nothing, and is ignored.
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
     * Records the declaration in the handler's own declarations. The RIB reader keeps its own
     * for the file it reads, and this one as well.
     **/
    void declare(const v3d::render::offline::rib::requests::Declare & request) override;

    void option(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void hider(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;

    void format(const v3d::render::offline::rib::requests::Format & request) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(const v3d::render::offline::rib::requests::ScreenWindow & request) override;
    void cropWindow(const v3d::render::offline::rib::requests::CropWindow & request) override;
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void clipping(const v3d::render::offline::rib::requests::Clipping & request) override;
    void depthOfField(const v3d::render::offline::rib::requests::DepthOfField & request) override;
    void shutter(const v3d::render::offline::rib::requests::Shutter & request) override;
    void pixelSamples(const glm::uvec2 & samples) override;
    void pixelFilter(v3d::render::offline::film::Filter filter, float xwidth, float ywidth) override;
    void pixelVariance(float variation) override;
    void display(const v3d::render::offline::rib::requests::Display & request,
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
    void basis(const v3d::render::offline::rib::requests::Basis & request) override;

    void surface(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void lightSource(const v3d::render::offline::rib::requests::LightSource & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void displacement(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void attribute(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void areaLightSource(const v3d::render::offline::rib::requests::LightSource & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    /** RenderMan's lights are the new core's alone, so the hiders log the first and draw none. **/
    void light(const v3d::render::offline::rib::requests::LightSource & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    /** RenderMan's shading networks are the new core's alone, so the hiders log the first. **/
    void pattern(const v3d::render::offline::rib::requests::ShadingNode & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void bxdf(const v3d::render::offline::rib::requests::ShadingNode & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void illuminate(const std::string & handle, bool on) override;
    void imager(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;

    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override;
    void pointsPolygons(const v3d::render::offline::rib::requests::PointsPolygons & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void curves(const v3d::render::offline::rib::requests::Curves & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    /**
     * Drawn by the ray hider only, because the reyes hider does not dice spheres. The first
     * sphere a scene names under the reyes hider is reported.
     **/
    void sphere(const v3d::render::offline::rib::requests::Sphere & request,
        const v3d::render::offline::rib::ParameterList & parameters) override;

    /**
     * The reyes and ray hiders draw none of these. The first of each a scene names is reported,
     * and the scene renders without them.
     **/
    void points(unsigned int count, const v3d::render::offline::rib::ParameterList & parameters) override;
    void cone(const v3d::render::offline::rib::requests::Cone & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void cylinder(const v3d::render::offline::rib::requests::Cylinder & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void hyperboloid(const v3d::render::offline::rib::requests::Hyperboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void paraboloid(const v3d::render::offline::rib::requests::Paraboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void disk(const v3d::render::offline::rib::requests::Disk & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void torus(const v3d::render::offline::rib::requests::Torus & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void subdivisionMesh(const v3d::render::offline::rib::requests::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters) override;

    /**
     * The hiders do not instance objects. The geometry inside an object's block is left out, as
     * RI leaves it out where it is declared, and the first object a scene names is reported.
     **/
    void objectBegin(const std::string & handle) override;
    void objectEnd() override;

    /**
     * The context the requests reach. Its framebuffer holds the picture once the file has
     * been read.
     **/
    state::RenderContext & context();

    /**
     * What the scene has declared, and what the standard declares for it.
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
    /**
     * A number from a scene as a count of pixels or micropolygons: at least one and at most
     * largestLimit. Anything else is refused.
     **/
    static std::optional<uint32_t> count(float value);

    /**
     * A picture side as the context takes it. A side too large for an int is held just above
     * largestResolution, which the context refuses.
     **/
    static int side(unsigned int value);

    /**
     * The deepest a scene may ask trace() to go. A shader that traces twice at every hit doubles
     * the rays at every level, and every level holds a shader on the stack.
     **/
    static constexpr float DEEPEST_TRACE = 16.0F;

    typedef v3d::render::offline::rib::ParameterList ParameterList;

    /**
     * Print Pixar's copyright notice the first time it is called in a process. A static's
     * initialiser runs once even when two threads reach it together.
     **/
    static void notice();

    /** Whether the primitive arriving now deforms one already in the motion block. **/
    bool deforms();

    /** Whether the primitive arriving now is left out: it deforms, or it is inside an object. **/
    bool skipped();

    /** Report the first request of a name that the hiders do not draw. **/
    void unsupported(const std::string & request);

    state::RenderContext context_;
    v3d::render::offline::rib::Declarations declarations_;
    std::string output_;
    unsigned int width_ = 0;
    unsigned int height_ = 0;
    bool spheres_ = false;
    bool motion_ = false;
    unsigned int primitives_ = 0;
    bool deformed_ = false;
    bool object_ = false;
    std::set<std::string> unsupported_;
};

};  // namespace v3d::moya::ri
