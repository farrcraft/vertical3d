/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/lighting/SceneUniforms.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>

#include <vulkan/vulkan.h>

#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>


namespace v3d::render::realtime::vulkan::renderer {

/**
 * The device side of a lit scene. It holds the cel and outline pipelines a registered model is
 * drawn with, the shadow pipeline it casts with, and a skinned variant of each.
 *
 * It also holds the scene set that the lit pass and the shadow pass bind at set 2. One storage
 * buffer in the scene set holds every joint palette drawn in the frame, so a skinned model
 * casts the pose it is drawn in. Another holds every object drawn in the frame, which a shader
 * reads by its instance index. A draw names where its objects start as its first instance,
 * and pushes nothing.
 *
 * Every pipeline here declares the camera at set 0, the albedo at set 1 in the Textures
 * material layout, and the scene at set 2. The shadow pipelines also declare a push constant,
 * the cascade a draw is drawn into, which the others do not. What they draw is linear light,
 * so the target they draw into must be an sRGB one, which encodes on store.
 *
 * Front faces are clockwise, because a model is wound counter clockwise seen from outside and
 * the cameras in api/type flip y into Vulkan's clip space.
 **/
class Lit final {
 public:
    /**
     * One object's entry in the scene's object buffer, laid out as shaders/lit/lit.glsl
     * declares the Object struct. An array of it in a std430 buffer has a stride of 96 bytes,
     * because the matrix aligns each entry to 16.
     **/
    struct Object final {
        glm::mat4 model;
        glm::vec4 baseColour;
        float outline;        /**< how far the outline hull is pushed out, and zero for the cel pass **/
        uint32_t firstJoint;  /**< where the object's palette starts, for a skinned pipeline **/
        uint32_t albedo = 0;  /**< the albedo's slot in the texture array at set 1, from slot() **/
        float padding = 0.0F;
    };

    /**
     * The SPIR-V each pipeline is built from. The defaults are the api's own, embedded at
     * build time. A game that loads its own shaders from disk passes in what it loaded.
     *
     * A replacement must declare the same descriptor sets as the defaults, which including
     * shaders/lit/lit.glsl provides. A shadow stage declares one push constant, a uint naming
     * the cascade, and no other stage declares any. A vertex stage reads its object at its
     * instance index, and a fragment stage at an index its vertex stage hands it. A missing or
     * extra binding fails at pipeline creation. A block with its members reordered is not
     * detected.
     *
     * A vertex stage is given the attributes its default reads and no others. The cel pass
     * gets the position, normal and uv, the outline the position and normal, and the shadow
     * pass the position alone. A skinned stage also gets the joints and weights.
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
     * @param textures whose layout set 1 is, and whose white texture stands in for a
     *        shadow map until a scene names one
     * @param colour the format of what the cel pass draws into, which must be an sRGB one
     * @param depth the format of that pass's depth
     * @param shadow the depth format of the target a shadow pass draws into, or
     *        VK_FORMAT_UNDEFINED for a scene that casts no shadow and builds no shadow pipeline
     * @param front the winding of a face that faces the camera, as it lands on screen. Clockwise
     *        is right for a camera on type::camera::Profile::Hand::UpCrossDirection, and
     *        counter clockwise for one on DirectionCrossUp, which mirrors the image. The cel,
     *        outline and shadow pipelines all cull by it, so the light's matrix is built in the
     *        camera's hand too: see lighting::shadowLight.
     * @throw std::runtime_error if a pipeline or the scene layout cannot be created
     **/
    Lit(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<pipeline::Cache>& cache,
        const boost::shared_ptr<pipeline::Resources>& resources, const boost::shared_ptr<frame::Ring>& ring,
        const boost::shared_ptr<frame::FrameUniforms>& uniforms, const boost::shared_ptr<Textures>& textures,
        VkFormat colour, VkFormat depth, VkFormat shadow, const Shaders& shaders = Shaders::embedded(),
        VkFrontFace front = VK_FRONT_FACE_CLOCKWISE);

    Lit(const Lit&) = delete;
    Lit& operator=(const Lit&) = delete;

    /**
     * @return the pipeline a model's surface is drawn with
     **/
    PipelineHandle cel() const noexcept;

    /**
     * @return where a texture sits in the texture array, for an Object's albedo. Read it each
     *         frame, because a texture filled or replaced later moves to another slot
     **/
    uint32_t slot(const TextureHandle& texture) const;

    /**
     * @return the pipeline a model's outline is drawn with, before its surface
     **/
    PipelineHandle outline() const noexcept;

    /**
     * The pipeline a caster is drawn into a shadow map with. It is built with depth bias, so
     * the pass it is drawn in must name one, from lighting::Settings::constantBias and
     * slopeBias. A draw with it pushes a Cascade.
     *
     * @return the pipeline, or an unset handle when this was built with no shadow format
     **/
    PipelineHandle shadow() const noexcept;

    /**
     * The push constant a draw with shadow() or skinnedShadow() carries: which cascade, and so
     * which layer and matrix, it is drawn into.
     **/
    struct Cascade final {
        uint32_t index = 0;
    };

    /**
     * The pipelines a skinned model is drawn with, in place of cel(), outline() and shadow().
     * Their vertex is a type::Model::Vertex followed by its type::Model::Influence, which is
     * MeshRegistry::SkinnedVertex.
     **/
    PipelineHandle skinnedCel() const noexcept;
    PipelineHandle skinnedOutline() const noexcept;
    PipelineHandle skinnedShadow() const noexcept;

    /**
     * Write this frame's scene and return the set a lit pass names as its scene.
     *
     * One scene a frame: the set is the frame's own, so a second call in the same frame
     * replaces what the first wrote, and empties the frame's objects. It waits for the frame's
     * last submission to finish, as StreamRing::claim() does. Call it while the frame is
     * built, before the Ring begins the frame, and before any object() call for the frame.
     *
     * @param shadowMap the depth the shadow passes drew, a sampled depth target of a layer
     *        per cascade, or an unset handle for no shadow
     * @param palette every joint matrix drawn this frame, from entity::Poses::palette()
     **/
    VkDescriptorSet scene(const lighting::SceneUniforms& uniforms, const TextureHandle& shadowMap = TextureHandle(),
        const std::vector<glm::mat4>& palette = {});

    /**
     * Add an object to the scene the last call to scene() wrote. A draw of it names the index
     * returned as its first instance.
     *
     * @return the object's index in the frame's object buffer
     * @throw std::runtime_error if scene() has not been called
     **/
    uint32_t object(const Object& object);

    /**
     * @return set 2's layout, for a game's own pipeline that reads the same scene
     **/
    VkDescriptorSetLayout sceneLayout() const noexcept;

 private:
    /**
     * A frame's scene: the uniform buffer, the palettes, the objects, and the set pointing at
     * all three.
     **/
    struct Slot final {
        boost::shared_ptr<memory::Buffer> buffer;
        boost::shared_ptr<memory::Buffer> palette;
        boost::shared_ptr<memory::Buffer> objects;
        uint32_t count = 0;   /**< the objects added since the frame's scene was written **/
        VkDescriptorSet set = VK_NULL_HANDLE;
    };

    /**
     * Which storage binding a buffer is for, how many bytes it needs, and how many bytes of
     * the buffer it replaces are copied into the new one.
     **/
    struct Reservation {
        uint32_t binding = 0;
        VkDeviceSize needed = 0;
        VkDeviceSize kept = 0;
    };

    /**
     * Point one of a slot's storage bindings at a buffer of at least this many bytes,
     * replacing the one it has when that is smaller. A replacement starts with a copy of the
     * old buffer's first kept bytes.
     *
     * @param buffer the slot's buffer for the binding
     **/
    void reserve(Slot* slot, boost::shared_ptr<memory::Buffer>* buffer, const Reservation& reservation);

    /**
     * Compile the cel and outline pipelines against the colour and depth formats, and the
     * shadow pipeline against its own depth format when there is one.
     **/
    void createPipelines(const Shaders& shaders, VkFormat colour, VkFormat depth, VkFormat shadow, VkFrontFace front);

    /**
     * The frame's slot, created the first time the frame asks for one.
     **/
    Slot& slot();

    boost::shared_ptr<device::Device> device_;
    boost::shared_ptr<pipeline::Cache> cache_;
    boost::shared_ptr<pipeline::Resources> resources_;
    boost::shared_ptr<frame::Ring> ring_;
    boost::shared_ptr<frame::FrameUniforms> uniforms_;
    boost::shared_ptr<Textures> textures_;
    boost::shared_ptr<pipeline::DescriptorPool> scenes_;
    std::vector<Slot> slots_;
    Slot* current_ = nullptr;   /**< the slot the last scene() wrote **/
    /**
     * A shadow map of one layer cleared to the far plane, which a scene with no shadow map
     * reads, so that nothing is in shadow
     **/
    boost::shared_ptr<frame::RenderTarget> unshadowed_;
    pipeline::Texture unshadowedTexture_;
    PipelineHandle cel_;
    PipelineHandle outline_;
    PipelineHandle shadow_;
    PipelineHandle skinnedCel_;
    PipelineHandle skinnedOutline_;
    PipelineHandle skinnedShadow_;
};

};  // namespace v3d::render::realtime::vulkan::renderer
