/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/LitSettings.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

#include "Quad.h"

namespace v3d::render::realtime::vulkan::renderer {

/**
 * The device half of a lit scene: the cel and outline pipelines a registered model is drawn
 * with, the shadow pipeline it casts with, a skinned variant of each, and the scene set both
 * passes bind at 2 - ADR-0064. The scene set carries every joint palette drawn in the frame, so
 * a skinned model casts the pose it is drawn in - ADR-0071.
 *
 * Every pipeline here declares the camera at set 0, the albedo at set 1 in renderer::Quad's
 * material layout, and the scene at set 2, with one push block for the object. What they
 * draw is linear light, so the target they draw into is an sRGB one - ADR-0066.
 *
 * Front faces are clockwise, because a model is wound counter clockwise seen from outside and
 * the cameras in api/type flip y into Vulkan's clip space - ADR-0012.
 **/
class Lit final {
 public:
    /**
     * The push block, laid out as shaders/lit/lit.glsl declares the Object block.
     **/
    struct Object final {
        glm::mat4 model;
        glm::vec4 baseColour;
        float outline;        /**< how far the outline hull is pushed out, and zero for the cel pass **/
        uint32_t firstJoint;  /**< where the object's palette starts, for a skinned pipeline **/
    };

    /**
     * The SPIR-V each pipeline is built from. The defaults are the api's own, embedded at
     * build time; a game that loads its own from disk hands over what it loaded - ADR-0067. A
     * replacement declares the blocks lit.glsl does.
     **/
    struct Shaders final {
        std::vector<uint32_t> mesh;             /**< the cel pass's vertex stage **/
        std::vector<uint32_t> cel;              /**< its fragment stage **/
        std::vector<uint32_t> outlineVertex;
        std::vector<uint32_t> outlineFragment;
        std::vector<uint32_t> shadow;           /**< the shadow pass's only stage **/
        std::vector<uint32_t> skinnedMesh;      /**< the vertex stages of the skinned variants, which **/
        std::vector<uint32_t> skinnedOutline;   /**< share the fragment stages above **/
        std::vector<uint32_t> skinnedShadow;

        /**
         * @return the api's shaders
         **/
        static Shaders embedded();
    };

    /**
     * @param quads whose material layout set 1 is, and whose white texture stands in for a
     *        shadow map until a scene names one
     * @param colour the format of what the cel pass draws into, which ADR-0066 makes an sRGB one
     * @param depth the format of that pass's depth
     * @param shadow the depth format of the target a shadow pass draws into, or
     *        VK_FORMAT_UNDEFINED for a scene that casts no shadow and builds no shadow pipeline
     * @throw std::runtime_error if a pipeline or the scene layout cannot be created
     **/
    Lit(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<pipeline::Cache>& cache,
        const boost::shared_ptr<pipeline::Resources>& resources, const boost::shared_ptr<frame::Ring>& ring,
        const boost::shared_ptr<frame::FrameUniforms>& uniforms, const boost::shared_ptr<Quad>& quads,
        VkFormat colour, VkFormat depth, VkFormat shadow, const Shaders& shaders = Shaders::embedded());

    Lit(const Lit&) = delete;
    Lit& operator=(const Lit&) = delete;

    /**
     * @return the pipeline a model's surface is drawn with
     **/
    PipelineHandle cel() const noexcept;

    /**
     * @return the pipeline a model's outline is drawn with, before its surface
     **/
    PipelineHandle outline() const noexcept;

    /**
     * The pipeline a caster is drawn into a shadow map with. It is built with depth bias, so
     * the pass it is drawn in names one - LitSettings::constantBias and slopeBias.
     *
     * @return the pipeline, or an unset handle when this was built with no shadow format
     **/
    PipelineHandle shadow() const noexcept;

    /**
     * The pipelines a skinned model is drawn with, in place of cel(), outline() and shadow().
     * Their vertex is a type::Model::Vertex followed by its type::Model::Influence -
     * MeshRegistry::SkinnedVertex.
     **/
    PipelineHandle skinnedCel() const noexcept;
    PipelineHandle skinnedOutline() const noexcept;
    PipelineHandle skinnedShadow() const noexcept;

    /**
     * Write this frame's scene and return the set a lit pass names as its scene.
     *
     * One scene a frame: the set is the frame's own, so a second call in the same frame
     * replaces what the first wrote. It waits for the frame it writes to have finished its last
     * submission, the way renderer::Quad does before writing its geometry, so it is called
     * while the frame is built and before the ring begins it.
     *
     * @param shadowMap the depth a shadow pass drew, or an unset handle for no shadow
     * @param palette every joint matrix drawn this frame - realtime::Poses::palette()
     **/
    VkDescriptorSet scene(const SceneUniforms& uniforms, const TextureHandle& shadowMap = TextureHandle(),
        const std::vector<glm::mat4>& palette = {});

    /**
     * @return set 2's layout, for a pipeline of a game's own that reads the same scene
     **/
    VkDescriptorSetLayout sceneLayout() const noexcept;

 private:
    /**
     * A frame's scene: the uniform buffer, the palettes, and the set pointing at both.
     **/
    struct Slot final {
        boost::shared_ptr<memory::Buffer> buffer;
        boost::shared_ptr<memory::Buffer> palette;
        VkDescriptorSet set = VK_NULL_HANDLE;
    };

    /**
     * Point a slot's set at a palette buffer large enough for this many matrices, replacing the
     * one it has when that is smaller.
     **/
    void reserve(Slot* slot, std::size_t joints);

    /**
     * Compile the cel and outline pipelines against the colour and depth formats, and the
     * shadow pipeline against its own depth format when there is one.
     **/
    void createPipelines(const Shaders& shaders, VkFormat colour, VkFormat depth, VkFormat shadow);

    /**
     * The frame's slot, created the first time the frame asks for one.
     **/
    Slot& slot();

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<pipeline::Cache> cache_;
    boost::shared_ptr<pipeline::Resources> resources_;
    boost::shared_ptr<frame::Ring> ring_;
    boost::shared_ptr<frame::FrameUniforms> uniforms_;
    boost::shared_ptr<Quad> quads_;
    boost::shared_ptr<pipeline::DescriptorPool> scenes_;
    std::vector<Slot> slots_;
    PipelineHandle cel_;
    PipelineHandle outline_;
    PipelineHandle shadow_;
    PipelineHandle skinnedCel_;
    PipelineHandle skinnedOutline_;
    PipelineHandle skinnedShadow_;
};

};  // namespace v3d::render::realtime::vulkan::renderer
