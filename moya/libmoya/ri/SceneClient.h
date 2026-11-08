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

#include <functional>
#include <map>
#include <string>
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
 * **Names.** An object takes its name from RI's `Attribute "identifier" "name"`. A display's
 * output is named after its file.
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

 private:
    /** RI's current transformation: one matrix, or a matrix at each of a motion block's times. **/
    struct Sampled {
        std::vector<float> times;
        std::vector<glm::mat4> matrices { glm::mat4(1.0F) };
    };

    /** What AttributeBegin saves and AttributeEnd restores. **/
    struct Attributes {
        Sampled transformation;
        std::string name;
    };

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

    Attributes current_;
    std::vector<Attributes> attributes_;
    std::vector<Sampled> transformations_;

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
