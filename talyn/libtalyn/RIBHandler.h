/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>
#include <vector>

#include "RenderContext.h"

#include "../../api/render/offline/RIBHandler.h"

#include <boost/shared_ptr.hpp>

namespace v3d::talyn {

/**
 * Builds a raytracer scene from a RIB stream, per ADR-0023 and ADR-0025.
 *
 * A scene is triangles in world space and one camera, so a polygon is fanned and its
 * points go through the current transformation on the way in. What the file says between
 * Projection and WorldBegin is the world to camera transformation, and the camera is
 * built from it there - every camera option is frozen at WorldBegin, which is what the
 * standard says happens.
 **/
class RIBHandler final : public v3d::render::offline::RIBHandler {
 public:
    explicit RIBHandler(const boost::shared_ptr<RenderContext> & rc);

    void format(unsigned int width, unsigned int height, float pixelAspect) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(float left, float right, float bottom, float top) override;
    void projection(const std::string & name, const v3d::render::offline::ParameterList & parameters) override;
    void clipping(float hither, float yon) override;

    void worldBegin() override;
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

    void polygon(unsigned int vertices, const v3d::render::offline::ParameterList & parameters) override;
    void pointsPolygons(const std::vector<unsigned int> & counts, const std::vector<unsigned int> & indices,
        const v3d::render::offline::ParameterList & parameters) override;

    /**
     * What the scene asked for that a raytracer built on v3d::type::Camera cannot do, or
     * empty. The reader still succeeds - the request was understood - so a caller that
     * wants a picture rather than a parse has to look here.
     **/
    const std::string & error() const;

 private:
    /**
     * What the camera options add up to, applied at WorldBegin.
     *
     * @return false, with error() set, when they name a camera a CameraProfile cannot
     *         hold: an off centre screen window, or a world to camera matrix that is not
     *         a rotation and a translation.
     **/
    bool buildCamera();

    /**
     * A face of the current polygon soup, fanned into triangles through the current
     * transformation. RI says a polygon is planar and convex, so a fan is the whole of it.
     **/
    void fan(const std::vector<glm::vec3> & points, const std::vector<unsigned int> & indices);

    /**
     * What RiAttributeBegin saves and RiAttributeEnd puts back.
     **/
    class Attributes {
     public:
        glm::mat4x4 transform = glm::mat4x4(1.0f);
        glm::vec3 color = glm::vec3(1.0f);
    };

    boost::shared_ptr<RenderContext> rc_;
    std::vector<glm::mat4x4> transforms_;
    std::vector<Attributes> attributes_;
    glm::mat4x4 transform_ = glm::mat4x4(1.0f);
    glm::vec3 color_ = glm::vec3(1.0f);
    std::string error_;
    std::string projection_ = "orthographic";
    // the RI defaults, and the same chain moya's render context follows: a format sets the
    // frame aspect, which sets the screen window, and naming either stops the one above it
    float screen_[4] = { -4.0f / 3.0f, 4.0f / 3.0f, -1.0f, 1.0f };
    float frameAspect_ = 4.0f / 3.0f;
    float fov_ = 90.0f;
    float near_ = 1.0e-10f;
    float far_ = 1.0e38f;
    bool frameAspectNamed_ = false;
    bool screenNamed_ = false;
    bool clippingNamed_ = false;
};

};  // namespace v3d::talyn
