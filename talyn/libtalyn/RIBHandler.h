/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/sl/ShaderLibrary.h>

#include <string>
#include <vector>

#include "RenderContext.h"

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
class RIBHandler final : public v3d::render::offline::rib::Handler {
 public:
    explicit RIBHandler(const boost::shared_ptr<RenderContext> & rc);

    void format(unsigned int width, unsigned int height, float pixelAspect) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(float left, float right, float bottom, float top) override;
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
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

    void option(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;

    void color(const glm::vec3 & value) override;
    void opacity(const glm::vec3 & value) override;

    void surface(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void lightSource(const std::string & name, const std::string & handle,
        const v3d::render::offline::rib::ParameterList & parameters) override;
    void illuminate(const std::string & handle, bool on) override;
    void imager(const std::string & name,
        const v3d::render::offline::rib::ParameterList & parameters) override;

    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override;
    void pointsPolygons(const std::vector<unsigned int> & counts, const std::vector<unsigned int> & indices,
        const v3d::render::offline::rib::ParameterList & parameters) override;

    /**
     * What the scene asked for that a raytracer built on v3d::type::camera::Camera cannot do, or
     * empty. The reader still succeeds - the request was understood - so a caller that
     * wants a picture rather than a parse has to look here.
     **/
    const std::string & error() const;

 private:
    /**
     * What the camera options add up to, applied at WorldBegin.
     *
     * @return false, with error() set, when they name a camera a Profile cannot
     *         hold: an off centre screen window, or a world to camera matrix that is not
     *         a rotation and a translation.
     **/
    bool buildCamera();

    /**
     * A face of the current polygon soup, fanned into triangles through the current
     * transformation. RI says a polygon is planar and convex, so a fan is the whole of it.
     **/
    void fan(const std::vector<glm::vec3> & points, const std::vector<glm::vec3> & normals,
        const std::vector<unsigned int> & indices);

    /**
     * What RiAttributeBegin saves and RiAttributeEnd puts back.
     **/
    class Attributes {
     public:
        glm::mat4x4 transform = glm::mat4x4(1.0f);
        glm::vec3 color = glm::vec3(1.0f);
        glm::vec3 opacity = glm::vec3(1.0f);
        v3d::render::offline::sl::Placed surface;
        /**
         * Which lights are switched on, by handle. The lights themselves belong to the
         * scene, because a light belongs to the frame rather than to the block that made
         * it - so an AttributeEnd puts this back and not them.
         **/
        std::vector<std::string> lit;
    };

    /**
     * A light the scene created, under the handle a later Illuminate names it by.
     *
     * They are held here rather than in the scene because the scene is given only the
     * ones that are on when a primitive needs them: a raytracer shades every triangle
     * against one light list, so there is one moment - the first primitive - at which
     * which lights are on stops being a question and becomes an answer.
     **/
    class LightSource {
     public:
        std::string handle;
        v3d::render::offline::sl::Placed light;
    };

    /**
     * The surface a triangle added now is shaded by, and the lights on it.
     *
     * The lights reach the scene here, the first time a primitive asks for them. A scene
     * that switches a light off after its geometry is a scene the standard does not
     * describe, and this renderer draws the lights that were on at the first primitive.
     **/
    v3d::render::offline::sl::Placed shading();

    boost::shared_ptr<RenderContext> rc_;
    boost::shared_ptr<v3d::render::offline::sl::ShaderLibrary> shaders_;
    std::vector<glm::mat4x4> transforms_;
    std::vector<Attributes> attributes_;
    std::vector<LightSource> lights_;
    std::vector<std::string> lit_;
    v3d::render::offline::sl::Placed surface_;
    glm::mat4x4 transform_ = glm::mat4x4(1.0f);
    glm::vec3 color_ = glm::vec3(1.0f);
    glm::vec3 opacity_ = glm::vec3(1.0f);
    /** Whether the scene's lights have been handed over, which happens once. **/
    bool lit_given_ = false;
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
