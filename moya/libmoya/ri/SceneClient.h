/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/log/Logger.h>
#include <api/render/offline/film/Sampling.h>
#include <api/render/offline/rib/Handler.h>
#include <api/render/offline/rib/Parameters.h>
#include <api/render/offline/rib/Requests.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <utility>
#include <vector>

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
 * zero, a distant light and a sphere light shaped by a cone. Each points down its `to` from its
 * `from`. A light names the instances made while it was on, or none when it lit every instance.
 * Another light shader is logged and makes no light. `AreaLightSource` makes a mesh light of
 * each geometry after it in its attribute block.
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

    void format(const v3d::render::offline::rib::Format & request) override;
    void frameAspectRatio(float aspect) override;
    void screenWindow(const v3d::render::offline::rib::ScreenWindow & request) override;
    void cropWindow(const v3d::render::offline::rib::CropWindow & request) override;
    /**
     * `"perspective"` or `"orthographic"`, with a perspective one's `"fov"`. Another projection
     * is logged and changes nothing.
     **/
    void projection(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void clipping(const v3d::render::offline::rib::Clipping & request) override;
    void depthOfField(const v3d::render::offline::rib::DepthOfField & request) override;
    void shutter(const v3d::render::offline::rib::Shutter & request) override;
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
    void display(const v3d::render::offline::rib::Display & request, const v3d::render::offline::rib::ParameterList & parameters) override;

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
    void basis(const v3d::render::offline::rib::Basis & request) override;
    void surface(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void displacement(const std::string & name, const v3d::render::offline::rib::ParameterList & parameters) override;
    void lightSource(const v3d::render::offline::rib::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void areaLightSource(const v3d::render::offline::rib::LightSource & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void illuminate(const std::string & handle, bool on) override;

    Handler* deformation() override;
    void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override;
    void pointsPolygons(const v3d::render::offline::rib::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void sphere(const v3d::render::offline::rib::Sphere & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void curves(const v3d::render::offline::rib::Curves & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void points(unsigned int count, const v3d::render::offline::rib::ParameterList & parameters) override;
    void cone(const v3d::render::offline::rib::Cone & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void cylinder(const v3d::render::offline::rib::Cylinder & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void hyperboloid(const v3d::render::offline::rib::Hyperboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void paraboloid(const v3d::render::offline::rib::Paraboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void disk(const v3d::render::offline::rib::Disk & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    void torus(const v3d::render::offline::rib::Torus & request, const v3d::render::offline::rib::ParameterList & parameters) override;
    /**
     * A tag RI names, `crease`, `corner`, `hole` or `interpolateboundary`, becomes the mesh's
     * parameters. Another tag is logged.
     **/
    void subdivisionMesh(const v3d::render::offline::rib::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters) override;

    void objectBegin(const std::string & handle) override;
    void objectEnd() override;
    /** An object no objectBegin() named is logged, and places nothing. **/
    void objectInstance(const std::string & handle) override;

 private:
    /** RI's current transformation: one matrix, or a matrix at each of a motion block's times. **/
    struct Sampled {
        std::vector<float> times;
        std::vector<glm::mat4> matrices { glm::mat4(1.0F) };
    };

    /** A shader as a request names it. **/
    struct Shader {
        std::string name;
        v3d::render::offline::rib::ParameterList parameters;
    };

    /** What AttributeBegin saves and AttributeEnd restores. **/
    struct Attributes {
        Sampled transformation;
        std::string name;
        glm::vec3 colour { 1.0F };
        glm::vec3 opacity { 1.0F };
        Shader surface { .name = "defaultsurface" };
        Shader displacement;

        /** The material the state above makes, or none until a geometry needs it. **/
        scene::Handle material;

        /** The lights that are on, by handle, and the area light the geometry after it makes. **/
        std::set<std::string> lights;
        std::optional<Shader> area;

        std::string basis { "bezier" };

        /** The group an object's geometry goes in, or none in the world. **/
        scene::Handle group;
    };

    /**
     * A primitive as the scene holds it: its kind, the request that made it, its parameters, and
     * the parameters of the request that become primvars.
     **/
    struct Shape {
        std::string kind;
        std::string request;
        std::vector<std::pair<std::string, scene::Value>> parameters;
        v3d::render::offline::rib::ParameterList primvars;
        std::set<std::string> taken;
    };

    /**
     * Takes each pose after the first of a deforming primitive, and adds its samples to the
     * prototype the first pose made.
     **/
    class Deforming final : public v3d::render::offline::rib::Handler {
     public:
        explicit Deforming(SceneClient * client);
        void polygon(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters) override;
        void pointsPolygons(const v3d::render::offline::rib::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void sphere(const v3d::render::offline::rib::Sphere & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void curves(const v3d::render::offline::rib::Curves & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void points(unsigned int count, const v3d::render::offline::rib::ParameterList & parameters) override;
        void cone(const v3d::render::offline::rib::Cone & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void cylinder(const v3d::render::offline::rib::Cylinder & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void hyperboloid(const v3d::render::offline::rib::Hyperboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void paraboloid(const v3d::render::offline::rib::Paraboloid & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void disk(const v3d::render::offline::rib::Disk & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void torus(const v3d::render::offline::rib::Torus & request, const v3d::render::offline::rib::ParameterList & parameters) override;
        void subdivisionMesh(const v3d::render::offline::rib::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters) override;

     private:
        SceneClient * client_;
    };

    /** A subdivision mesh's creases, corners, holes and boundary rule, as its tags give them. **/
    struct Edges {
        std::vector<std::int32_t> creases;
        std::vector<std::int32_t> creaseLengths;
        std::vector<float> creaseSharpness;
        std::vector<std::int32_t> corners;
        std::vector<float> cornerSharpness;
        std::vector<std::int32_t> holes;
        std::string boundary { "none" };
    };

    /**
     * RenderMan's interpolateboundary rule: 0 for none, 2 for the edges alone, and any other for
     * the edges and the corners.
     **/
    static std::string boundaryOf(unsigned int rule);

    /**
     * Add one tag's integers and floats to a mesh's edges.
     *
     * @return false for a tag RI does not name
     **/
    static bool addTag(const std::string & name, const std::vector<unsigned int> & given, const std::vector<float> & sharpness, Edges * edges);

    /** A light the world made, and the instances made while it was on. **/
    struct Light {
        scene::Handle light;
        std::vector<scene::Handle> lit;
    };

    /** The shapes of the primitive requests, which place() and deform() take. **/
    static Shape polygonShape(unsigned int vertices, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape meshShape(const v3d::render::offline::rib::PointsPolygons & request, const v3d::render::offline::rib::ParameterList & parameters);
    Shape curvesShape(const v3d::render::offline::rib::Curves & request, const v3d::render::offline::rib::ParameterList & parameters) const;
    static Shape pointsShape(const v3d::render::offline::rib::ParameterList & parameters);
    Shape subdivisionShape(const v3d::render::offline::rib::SubdivisionMesh & request, const v3d::render::offline::rib::ParameterList & parameters);
    static Shape quadricShape(const std::string & kind, const std::string & request, const v3d::render::offline::rib::ParameterList & parameters,
        const std::vector<std::pair<std::string, scene::Value>> & dimensions);

    /**
     * Make a shape's prototype and an instance of it at the current state. A primitive outside
     * the world is logged once and makes nothing.
     **/
    void place(const Shape & shape);

    /** Add a deforming pose's samples to the prototype the motion block's first pose made. **/
    void deform(const Shape & shape);

    /** An instance of a prototype at the current state, lit by the lights that are on. **/
    scene::Handle instance(const scene::Handle & prototype, const std::string & name, bool lit);

    /** The material of the current attribute state, made when there is none. **/
    scene::Handle material();

    /** A shader node of a request's shader, language and parameters. **/
    scene::Handle shaderNode(const Shader & shader, const std::string & name);

    /** A light at the current transformation, from `from` towards `to`. **/
    scene::Handle light(const std::string & kind, const std::string & name, const v3d::render::offline::rib::ParameterList & parameters,
        const glm::vec3 & defaultTo);

    /** A name for a new object of a kind: the attribute state's, or the request's and a count. **/
    std::string nameFor(const std::string & kind, const std::string & request);

    /** A name no object of the kind holds: the one given, or it with a number after it. **/
    std::string unique(const std::string & kind, const std::string & base);

    /** Make an object in the world, which the next world removes. **/
    scene::Handle make(const std::string & kind, const std::string & name);

    /** Log a message once per key. **/
    void once(const std::string & key, const std::string & message);

    /** A transform request that concatenates, or one that replaces the transformation. **/
    void concat(const glm::mat4 & matrix);
    void replace(const glm::mat4 & matrix);

    /**
     * Set a parameter that may move to the samples a transformation holds. Samples whose times
     * do not rise are logged, and the first is kept alone.
     **/
    void setSampled(const scene::Handle & object, const std::string & parameter, const Sampled & sampled);

    /** The output named after a display's file, made when there is none. **/
    scene::Handle output(const std::string & file);

    scene::Scene & scene_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    std::function<void()> committed_;

    scene::Handle options_;
    scene::Handle camera_;
    std::map<std::string, scene::Handle> outputs_;
    std::map<std::string, scene::Handle> channels_;

    Attributes current_;
    std::vector<Attributes> attributes_;
    std::vector<Sampled> transformations_;

    bool world_ { false };
    std::vector<scene::Handle> made_;
    std::vector<scene::Handle> instances_;
    std::map<std::string, Light> lights_;

    /** The handles of lights a scene declared and the client made nothing of. **/
    std::set<std::string> unmade_;
    std::map<std::string, scene::Handle> objects_;
    std::map<std::string, Sampled> spaces_;
    std::map<std::string, unsigned int> counts_;
    std::map<std::string, std::set<std::string>> taken_;
    std::set<std::string> logged_;

    /** The prototype a motion block's first pose made, and the samples of its poses. **/
    scene::Handle deforming_;
    std::map<std::string, std::vector<scene::Value>> samples_;
    Deforming deformations_ { this };

    /**
     * An open motion block's times, the transformation it opened with, and each time's
     * request.
     **/
    bool motion_ { false };
    std::vector<float> times_;
    Sampled base_;
    std::vector<glm::mat4> poses_;
};

};  // namespace v3d::moya::ri
