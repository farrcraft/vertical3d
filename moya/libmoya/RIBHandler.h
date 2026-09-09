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
 * Drives a reyes render context from a RIB stream, per ADR-0023 and ADR-0025.
 *
 * RiBegin and RiEnd have no RIB equivalent - the standard says they are implied at the
 * start and end of a file - so a handler creates the context it drives and destroys it
 * when it goes out of scope.
 *
 * The C entry points in RenderMan.cxx are the other way into the same render context.
 * Neither goes through the other: a va_list cannot be built at runtime, so a reader
 * holding a parsed parameter list could not call them.
 **/
class RIBHandler final : public v3d::render::offline::rib::Handler {
 public:
    explicit RIBHandler(Renderer * renderer);
    ~RIBHandler();

    void option(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;

    void format(unsigned int width, unsigned int height, float pixelAspect) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(float left, float right, float bottom, float top) override;
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void clipping(float hither, float yon) override;
    void display(const std::string & name, const std::string & type, const std::string & mode,
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
    void rotate(float angle, float dx, float dy, float dz) override;
    void scale(float sx, float sy, float sz) override;

    void color(const glm::vec3 & value) override;
    void opacity(const glm::vec3 & value) override;
    void shadingRate(float size) override;

    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override;
    void pointsPolygons(const std::vector<unsigned int> & counts, const std::vector<unsigned int> & indices,
        const v3d::render::offline::rib::ParameterList & parameters) override;

    /**
     * The context the requests are landing in, which is where a driver reads the
     * framebuffer from once the file has been read.
     **/
    RenderContext & context();

    /**
     * Where the render goes whatever the scene's Display names.
     *
     * A command line naming an output file means that file rather than the one the scene
     * was written to write, and a scene's Display arrives after this is set.
     **/
    void output(const std::string & name);

 private:
    Renderer * renderer_;
    std::string output_;
};

};  // namespace v3d::moya
