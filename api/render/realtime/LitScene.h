/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Image.h>
#include <api/render/realtime/Grade.h>
#include <api/render/realtime/Handle.h>
#include <api/render/realtime/MeshRegistry.h>
#include <api/render/realtime/Pass.h>
#include <api/render/realtime/RenderTarget.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/entity/Poses.h>
#include <api/render/realtime/lighting/Settings.h>
#include <api/render/realtime/lighting/Shadow.h>
#include <api/render/realtime/renderer/Lit.h>

#include <vulkan/vulkan.h>

#include <cstdint>
#include <string>
#include <variant>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace v3d::render::realtime {

class DeviceContext;
class Frame;

/**
 * A lit scene: its shadow map, the target it is drawn into, the lit renderer and the grade
 * that carries it into the app's own pass.
 *
 * draw() adds a frame's passes in an order the frame records correctly however it was built.
 * Each shadow pass draws one cascade into its layer of the map. The lit pass reads the map,
 * draws the registry's meshes into the scene target, and names the scene both kinds of pass
 * share. The grade is submitted to the pass the caller hands in, which reads the scene target.
 * Every rule the three passes have to keep between them is kept here. The scene is written
 * before any draw names it. Each shadow pass names its depth bias, its layer and its cascade.
 * Each reader names what it reads.
 *
 * The passes are named after the scene, so two scenes drawn into one frame have passes of
 * their own.
 **/
class LitScene final {
 public:
    /**
     * What the scene's targets and pipelines are built as.
     **/
    struct Spec final {
        std::string name = "lit";  /**< the lit pass's name, which the shadow passes' names start with **/
        uint32_t width = 0;        /**< the scene target's size in pixels **/
        uint32_t height = 0;
        VkFormat colour = VK_FORMAT_R8G8B8A8_SRGB;  /**< the scene target's colour, which has to be sRGB **/
        /** the format of what the grade draws into, such as DeviceContext::colourFormat() **/
        VkFormat output = VK_FORMAT_UNDEFINED;
        /** the depth format of the pass the grade is drawn in, or undefined for none **/
        VkFormat outputDepth = VK_FORMAT_UNDEFINED;
        /** each layer of the shadow map's width in texels, or zero for a scene with no shadow **/
        uint32_t shadowTexels = 2048;
        /** how many cascades the map holds a layer for, 1 to lighting::maximumCascades **/
        uint32_t cascades = 1;
        glm::vec4 clearColour{0.0F, 0.0F, 0.0F, 1.0F};  /**< what the lit pass clears to, in linear light **/
        boost::shared_ptr<image::Image> grade;            /**< the grade's table, or empty for the identity **/
        renderer::Lit::Shaders shaders = renderer::Lit::Shaders::embedded();
        /** the winding of a face towards the camera, as renderer::Lit::Spec names it **/
        VkFrontFace front = VK_FRONT_FACE_CLOCKWISE;
    };

    /**
     * What a frame's shadows are drawn through. Nothing draws no shadow. One matrix, from
     * lighting::shadowLight(), is one map that every caster is drawn into. Cascades, from
     * lighting::fitCascades(), draw a pass for each, and leave out a static caster that cannot
     * reach a cascade.
     **/
    using Shadows = std::variant<std::monostate, glm::mat4, lighting::Cascades>;

    /**
     * What one frame of the scene is drawn from.
     **/
    struct Shot final {
        glm::mat4 view{1.0F};
        glm::mat4 projection{1.0F};
        lighting::Settings settings;
        Shadows shadows;
        float alpha = 1.0F;  /**< engine::Engine::alpha(), how far the frame is between two steps **/
        /** this frame's poses of the skinned entities, or empty for static models only **/
        entity::Poses poses;
    };

    /**
     * @param context the device and the stores the scene is built on. The scene holds what it
     *        uses of them, and not the context
     * @throw std::runtime_error if a target or a pipeline cannot be built, if the size is zero,
     *        if no output format is named, or for cascades outside 1 to maximumCascades
     **/
    LitScene(const DeviceContext& context, const Spec& spec);

    /**
     * Release the grade's source and the shadow map's registration through the ring, so a frame
     * in flight finishes with them.
     **/
    ~LitScene();

    LitScene(const LitScene&) = delete;
    LitScene& operator=(const LitScene&) = delete;

    /**
     * Replace the scene target with one of a new size, as a window that changed size needs.
     * The grade reads the new target from the next frame drawn.
     *
     * @throw std::runtime_error if either dimension is zero, and the scene keeps its old target
     **/
    void resize(uint32_t width, uint32_t height);

    /**
     * Add this frame's passes, and draw the registry's meshes into them.
     *
     * Call it once a frame, while the frame is built, and before anything else is submitted to
     * the pass the grade goes into, since the grade covers the whole of it.
     *
     * @param into the pass the graded scene is drawn into, such as the window's colour pass. It
     *        is the caller's, and draws into something of the output format
     * @return the lit pass, for anything else the scene draws after its meshes, such as world
     *         quads built against target()'s formats
     * @throw std::runtime_error if the shot names more cascades than the map has layers, or
     *        names shadows for a scene built with none
     **/
    Pass* draw(Frame* frame, Pass* into, const entt::registry& registry, const MeshRegistry& meshes, const Shot& shot);

    /**
     * @return the target the lit pass draws into
     **/
    const RenderTarget& target() const noexcept;

    /**
     * @return the lit renderer, for an app that adds draws of its own to the lit pass
     **/
    renderer::Lit& lit() noexcept;

    /**
     * @return the grade, for an app that changes its table
     **/
    Grade& grade() noexcept;

 private:
    std::string name_;
    glm::vec4 clearColour_;
    boost::shared_ptr<Textures> textures_;
    boost::shared_ptr<RenderTarget> scene_;
    /**< null for a scene with no shadow **/
    boost::shared_ptr<RenderTarget> map_;
    TextureHandle mapTexture_;
    renderer::Lit lit_;
    Grade grade_;
    MaterialHandle source_;
};

};  // namespace v3d::render::realtime
