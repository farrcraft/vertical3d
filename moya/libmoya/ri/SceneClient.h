/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/requests/Basis.h>
#include <api/render/offline/rib/requests/Clipping.h>
#include <api/render/offline/rib/requests/Cone.h>
#include <api/render/offline/rib/requests/CropWindow.h>
#include <api/render/offline/rib/requests/Curves.h>
#include <api/render/offline/rib/requests/Cylinder.h>
#include <api/render/offline/rib/requests/DepthOfField.h>
#include <api/render/offline/rib/requests/Disk.h>
#include <api/render/offline/rib/requests/Display.h>
#include <api/render/offline/rib/requests/Format.h>
#include <api/render/offline/rib/requests/Hyperboloid.h>
#include <api/render/offline/rib/requests/LightSource.h>
#include <api/render/offline/rib/requests/MakeTexture.h>
#include <api/render/offline/rib/requests/Paraboloid.h>
#include <api/render/offline/rib/requests/PointsPolygons.h>
#include <api/render/offline/rib/requests/ScreenWindow.h>
#include <api/render/offline/rib/requests/Shutter.h>
#include <api/render/offline/rib/requests/Sphere.h>
#include <api/render/offline/rib/requests/SubdivisionMesh.h>
#include <api/render/offline/rib/requests/Torus.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/ri/internal/Deforming.h>
#include <moya/libmoya/ri/internal/Lights.h>
#include <moya/libmoya/ri/internal/Objects.h>
#include <moya/libmoya/ri/internal/Sampled.h>
#include <moya/libmoya/ri/internal/Shape.h>
#include <moya/libmoya/ri/internal/Warnings.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Scene.h>

#include <cstddef>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace v3d::moya::ri {

/**
 * Fills moya's retained scene from RI requests. It keeps RI's graphics state, and creates and
 * modifies scene objects as the requests arrive. The reyes and ray hiders read RIB through
 * RIBHandler instead.
 *
 * **The frame.** The options, the camera and the displays become the scene's `options`,
 * `camera`, `output` and `channel` objects. The options and the camera are made when the client
 * is, named `options` and `camera`, so a scene that names neither has RI's defaults.
 *
 * **The transformation.** Each transform request applies to the current transformation, which
 * holds a sample at each time of a motion block. Inside a block, each request gives the
 * transformation at the block's next time, built on the transformation the block opened with.
 * Outside a block, a request applies to every sample, so what follows a block moves with it.
 * `WorldBegin` makes the current transformation the camera's, from world space to camera
 * space, and resets it.
 *
 * **Commits.** `WorldEnd` commits what the frame described, and calls the function frame()
 * was given. The scene's owner applies the batch there. A file of several frames is then a
 * scene that changes between commits.
 *
 * **The world.** Each primitive becomes a geometry prototype and an instance of it, at the
 * current transformation, with the material the attribute state gives. A primitive repeated
 * inside a motion block deforms: each later pose adds a sample to the prototype the first made.
 * An object is a group, whose geometry is placed by each instance of it. A new world removes
 * every object the world before it made.
 *
 * **Shaders.** The attribute state's surface and displacement shaders become a material of RI
 * shader nodes, made when the first geometry after a change needs one, so the instances under
 * one attribute state share it. Its colour and opacity are the surface node's `Cs` and `Os`.
 *
 * **Lights.** RI's `pointlight`, `distantlight` and `spotlight` become a sphere light of radius
 * zero, a distant light and a sphere light of radius zero shaped by a cone. Each points down its
 * `to` from its `from`. A sphere light of radius zero is normalised, with four times RI's
 * intensity, so its radiant intensity is RI's. A light that lit every instance keeps
 * `includeRoot`. Any other clears it and names the instances made while it was on, which may be
 * none. Another light shader is logged and makes no light. `AreaLightSource` makes a mesh light
 * of each geometry after it in its attribute block.
 *
 * **RenderMan's lights.** `Light` makes UsdLux's light of the same shape from `PxrRectLight`,
 * `PxrDiskLight`, `PxrSphereLight`, `PxrCylinderLight`, `PxrDistantLight` and
 * `PxrDomeLight`, which is on as a `LightSource` is. `PxrMeshLight` makes a mesh light of each
 * geometry after it in its attribute block, as `AreaLightSource` does. Another shader is logged
 * and makes no light.
 *
 * **Names.** An object takes its name from RI's `Attribute "identifier" "name"`, or from its
 * request and its place in the file. A name a kind already holds gets a number after it. A
 * display's output is named after its file, and a light without a name after its handle.
 **/
class SceneClient final : public v3d::render::offline::rib::Handler {
 public:
    /**
     * @param scene made with scene::kinds(), which outlives the client
     **/
    SceneClient(scene::Scene & scene, const boost::shared_ptr<v3d::log::Logger> & logger);
    SceneClient(const SceneClient &) = delete;
    SceneClient & operator=(const SceneClient &) = delete;

    /** What is called after each frame is committed. An empty function calls nothing. **/
    void frame(const std::function<void()> & committed);

    void format(const v3d::render::offline::rib::requests::Format & request) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(const v3d::render::offline::rib::requests::ScreenWindow & request) override;
    void cropWindow(const v3d::render::offline::rib::requests::CropWindow & request) override;
    /**
     * `"perspective"` or `"orthographic"`, with a perspective one's `"fov"`. Another projection
     * is logged and changes nothing.
     **/
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void clipping(const v3d::render::offline::rib::requests::Clipping & request) override;
    void depthOfField(const v3d::render::offline::rib::requests::DepthOfField & request) override;
    void shutter(const v3d::render::offline::rib::requests::Shutter & request) override;
    /** The samples per pixel are the two counts multiplied. **/
    void pixelSamples(const glm::uvec2 & samples) override;
    void pixelFilter(v3d::render::offline::film::Filter filter, float xwidth, float ywidth) override;
    void pixelVariance(float variation) override;
    /**
     * A display whose name starts with `+` adds an output. Any other replaces every output
     * before it, as RI says. A file ending in `.png` is written as PNG, and any other as EXR,
     * unless the type is `"framebuffer"`. A mode of `"rgb"` or `"rgba"` writes the beauty alone.
     * Any other mode adds a channel of that source, named after it.
     **/
    void display(const v3d::render::offline::rib::requests::Display & request, const v3d::render::offline::rib::ParameterList & parameters) override;

    /**
     * Makes the texture file at once, as texture::make() does, in the colour space the
     * parameter `"string colorspace"` names, or the image's own. A wrap mode or filter RI does
     * not name is logged, and the texture is made with black or the box. A texture that cannot
     * be made is logged, and the file reads on.
     **/
    void makeTexture(const v3d::render::offline::rib::requests::MakeTexture & request, const v3d::render::offline::rib::ParameterList & parameters) override;

    void frameBegin(int frame) override;
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

    /** `"identifier"`'s `"name"` names the objects made after it in the attribute block. **/
    void attribute(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;

    /**
     * A space a scene names, or `"world"` and `"camera"`. Another of RI's own spaces is logged
     * and changes nothing.
     **/
    void coordinateSystem(const std::string & space) override;
    void coordSysTransform(const std::string & space) override;

    void color(const glm::vec3 & value) override;
    void opacity(const glm::vec3 & value) override;
    /** The basis a cubic curve takes. A basis RI does not name is logged, and Bezier is used. **/
    void basis(const v3d::render::offline::rib::requests::Basis & request) override;
    void surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void displacement(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void lightSource(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void areaLightSource(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void light(const v3d::render::offline::rib::requests::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void illuminate(const std::string & handle, bool on) override;

    Handler* deformation() override;
    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override;
    void pointsPolygons(const v3d::render::offline::rib::requests::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void sphere(const v3d::render::offline::rib::requests::Sphere & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void curves(const v3d::render::offline::rib::requests::Curves & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void points(unsigned int count, const v3d::render::offline::rib::ParameterList & parameters) override;
    void cone(const v3d::render::offline::rib::requests::Cone & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void cylinder(const v3d::render::offline::rib::requests::Cylinder & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void hyperboloid(const v3d::render::offline::rib::requests::Hyperboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void paraboloid(const v3d::render::offline::rib::requests::Paraboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void disk(const v3d::render::offline::rib::requests::Disk & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void torus(const v3d::render::offline::rib::requests::Torus & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    /**
     * A tag RI names, `crease`, `corner`, `hole` or `interpolateboundary`, becomes the mesh's
     * parameters. Another tag is logged.
     **/
    void subdivisionMesh(const v3d::render::offline::rib::requests::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters) override;

    void objectBegin(const std::string & handle) override;
    void objectEnd() override;
    /** An object no objectBegin() named is logged, and places nothing. **/
    void objectInstance(const std::string & handle) override;

 private:
    /** RI's default picture, which a side of zero in a Format asks for. **/
    static constexpr int kWidth = 320;
    static constexpr int kHeight = 240;

    /** Whether a file name ends with a suffix, compared exactly. **/
    static bool endsWith(const std::string & text, const std::string & suffix);

    /**
     * A shader as a request names it. An area light's name is the light's, and its light is
     * the shader that made it.
     **/
    struct Shader {
        std::string name{};
        v3d::render::offline::rib::ParameterList parameters{};
        std::string light{};
    };

    /** What AttributeBegin saves and AttributeEnd restores. **/
    struct Attributes {
        internal::Sampled transformation{};
        std::string name{};
        glm::vec3 colour { 1.0F };
        glm::vec3 opacity { 1.0F };
        Shader surface { .name = "defaultsurface" };
        Shader displacement{};

        /**
         * The material the state above makes, or none until a geometry needs it. AttributeBegin
         * shares the slot, so a material made inside a block is the one its parent's state
         * makes too. A change of shading gives the state a slot of its own.
         **/
        boost::shared_ptr<scene::Handle> material { boost::make_shared<scene::Handle>() };

        /** The lights that are on, by handle, and the area light the geometry after it makes. **/
        std::set<std::string> lights{};
        std::optional<Shader> area{};

        std::string basis { "bezier" };

        /** The group an object's geometry goes in, or none in the world. **/
        scene::Handle group{};
    };

    /**
     * Make a shape's prototype and an instance of it at the current state. A primitive outside
     * the world is logged once and makes nothing.
     **/
    void place(const internal::Shape & shape);

    /** An instance of a prototype at the current state, lit by the lights that are on. **/
    scene::Handle instance(const scene::Handle & prototype, const std::string & name, bool lit);

    /** The material of the current attribute state, made when there is none. **/
    scene::Handle material();

    /** A shader node of a request's shader, language and parameters. **/
    scene::Handle shaderNode(const Shader & shader, const std::string & name);

    /** A name for a new object of a kind: the attribute state's, or the request's and a count. **/
    std::string nameFor(const std::string & kind, const std::string & request);

    /** A transform request that concatenates, or one that replaces the transformation. **/
    void concat(const glm::mat4 & matrix);
    void replace(const glm::mat4 & matrix);

    /** The output named after a display's file, made when there is none. **/
    scene::Handle output(const std::string & file);

    scene::Scene & scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::function<void()> committed_;
    internal::Warnings warnings_;

    /** The built-in colour configuration textures are made in, made with the first texture. **/
    std::optional<colour::Config> colours_;

    scene::Handle options_;
    scene::Handle camera_;
    std::map<std::string, scene::Handle> outputs_;
    std::map<std::string, scene::Handle> channels_;

    Attributes current_;
    std::vector<Attributes> attributes_;
    // how deep the attribute stack was at WorldBegin, which WorldEnd unwinds it to
    std::size_t worldAttributes_ { 0 };
    std::vector<internal::Sampled> transformations_;

    bool world_ { false };
    internal::Objects made_;
    internal::Lights lights_;
    std::map<std::string, scene::Handle> objects_;
    std::map<std::string, internal::Sampled> spaces_;

    /** Takes the later poses of a deforming primitive. **/
    internal::Deforming deformations_;

    /**
     * An open motion block's times, the transformation it opened with, and each time's
     * request.
     **/
    bool motion_ { false };
    std::vector<float> times_;
    internal::Sampled base_;
    std::vector<glm::mat4> poses_;
};

};  // namespace v3d::moya::ri
