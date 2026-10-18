/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lit.h"

#include <api/render/realtime/Handle.h>
#include <api/render/realtime/Textures.h>
#include <api/render/realtime/lighting/SceneUniforms.h>
#include <api/render/realtime/vulkan/device/Device.h>
#include <api/render/realtime/vulkan/frame/FrameUniforms.h>
#include <api/render/realtime/vulkan/frame/Ring.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/render/realtime/vulkan/pipeline/Cache.h>
#include <api/render/realtime/vulkan/pipeline/DescriptorPool.h>
#include <api/render/realtime/vulkan/pipeline/Resources.h>
#include <api/render/realtime/vulkan/pipeline/Texture.h>
#include <api/type/Model.h>

#include <cstddef>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::renderer {

namespace {

// the stride of lit.glsl's Object struct in a std430 array
constexpr std::size_t OBJECT_STRIDE = 96;
static_assert(sizeof(Lit::Object) == OBJECT_STRIDE, "Lit::Object must match the std430 stride of lit.glsl's Object");

/**
 * The api's lit shaders, compiled to SPIR-V at build time - see v3d_add_shader.
 **/
const uint32_t meshShader[] =
#include "shaders/mesh.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t celShader[] =
#include "shaders/cel.frag.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t outlineVertexShader[] =
#include "shaders/outline.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t outlineFragmentShader[] =
#include "shaders/outline.frag.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t shadowShader[] =
#include "shaders/shadow.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t skinnedMeshShader[] =
#include "shaders/skinned_mesh.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t skinnedOutlineShader[] =
#include "shaders/skinned_outline.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t skinnedShadowShader[] =
#include "shaders/skinned_shadow.vert.inc"
;  // NOLINT(whitespace/semicolon)

template <std::size_t N>
std::vector<uint32_t> words(const uint32_t (&code)[N]) {
    return std::vector<uint32_t>(std::begin(code), std::end(code));
}

/**
 * Sets 0 to 2, which every lit pipeline declares alike so that the sets a pass binds stay bound
 * across a switch between them.
 **/
void declare(pipeline::Builder* builder, VkDescriptorSetLayout camera, VkDescriptorSetLayout material,
    VkDescriptorSetLayout scene) {
    builder->set(camera)
        .set(material)
        .set(scene);
}

/**
 * The vertex a pipeline reads: a model's, and for a skinned one its influence after it - the
 * layout MeshRegistry uploads a skinned model in. A depth only stage reads the position alone.
 **/
void vertex(pipeline::Builder* builder, bool skinned, bool positionOnly) {
    using Vertex = type::Model::Vertex;
    using Influence = type::Model::Influence;
    builder->vertexBinding({.binding = 0, .stride = static_cast<uint32_t>(skinned ? sizeof(Vertex) + sizeof(Influence) : sizeof(Vertex))})
        .vertexAttribute({.location = 0, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, position)});
    if (!positionOnly) {
        builder->vertexAttribute({.location = 1, .binding = 0, .format = VK_FORMAT_R32G32B32_SFLOAT, .offset = offsetof(Vertex, normal)})
            .vertexAttribute({.location = 2, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, uv)});
    }
    if (skinned) {
        builder->vertexAttribute({.location = 3, .binding = 0, .format = VK_FORMAT_R16G16B16A16_UINT, .offset = sizeof(Vertex) + offsetof(Influence, joints)})
            .vertexAttribute({.location = 4, .binding = 0, .format = VK_FORMAT_R32G32B32A32_SFLOAT, .offset = sizeof(Vertex) + offsetof(Influence, weights)});
    }
}

/**
 * The palette's starting room, in matrices: enough for a handful of characters before the
 * first grow.
 **/
const std::size_t initialJoints = 256;

/**
 * The object buffer's starting room: a scene of a few hundred objects before the first grow.
 **/
const std::size_t initialObjects = 256;

};  // namespace

/**
 **/
Lit::Shaders Lit::Shaders::embedded() {
    Shaders shaders;
    shaders.mesh = words(meshShader);
    shaders.cel = words(celShader);
    shaders.outlineVertex = words(outlineVertexShader);
    shaders.outlineFragment = words(outlineFragmentShader);
    shaders.shadow = words(shadowShader);
    shaders.skinnedMesh = words(skinnedMeshShader);
    shaders.skinnedOutline = words(skinnedOutlineShader);
    shaders.skinnedShadow = words(skinnedShadowShader);
    return shaders;
}

/**
 **/
Lit::Lit(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<pipeline::Cache>& cache,
    const boost::shared_ptr<pipeline::Resources>& resources, const boost::shared_ptr<frame::Ring>& ring,
    const boost::shared_ptr<frame::FrameUniforms>& uniforms, const boost::shared_ptr<Textures>& textures,
    VkFormat colour, VkFormat depth, VkFormat shadow, const Shaders& shaders, VkFrontFace front) :
    device_(device),
    cache_(cache),
    resources_(resources),
    ring_(ring),
    uniforms_(uniforms),
    textures_(textures) {
    VkDescriptorSetLayoutBinding block{};
    block.binding = 0;
    block.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    block.descriptorCount = 1;
    // the shadow pass reads the light's matrix in its vertex stage, and the cel pass the rest
    block.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding map{};
    map.binding = 1;
    map.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    map.descriptorCount = 1;
    map.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    VkDescriptorSetLayoutBinding palette{};
    palette.binding = 2;
    palette.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    palette.descriptorCount = 1;
    palette.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    // the vertex stage reads the model matrix and the outline, and the cel pass's fragment stage
    // the base colour
    VkDescriptorSetLayoutBinding objects{};
    objects.binding = 3;
    objects.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    objects.descriptorCount = 1;
    objects.stageFlags = VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT;

    // a set per frame in flight, which is all a scene ever needs
    scenes_ = boost::make_shared<pipeline::DescriptorPool>(device_, ring_,
        std::vector<VkDescriptorSetLayoutBinding>{block, map, palette, objects}, ring_->framesInFlight(), "scene");
    slots_.resize(ring_->framesInFlight() > 0 ? ring_->framesInFlight() : 1);

    createPipelines(shaders, colour, depth, shadow, front);
}

/**
 **/
void Lit::createPipelines(const Shaders& shaders, VkFormat colour, VkFormat depth, VkFormat shadow, VkFrontFace front) {
    const auto bytes = [](const std::vector<uint32_t>& code) { return code.size() * sizeof(uint32_t); };

    for (const bool skinned : {false, true}) {
        const std::vector<uint32_t>& meshVertex = skinned ? shaders.skinnedMesh : shaders.mesh;
        pipeline::Builder cel(device_);
        cel.name(skinned ? "lit-cel-skinned" : "lit-cel")
            .shader(VK_SHADER_STAGE_VERTEX_BIT, meshVertex.data(), bytes(meshVertex))
            .shader(VK_SHADER_STAGE_FRAGMENT_BIT, shaders.cel.data(), bytes(shaders.cel));
        vertex(&cel, skinned, false);
        cel.cull(VK_CULL_MODE_BACK_BIT, front)
            .depth(true, true)
            // a lit surface is opaque, and blending it would cost bandwidth on every fragment
            .blend(false)
            .colourFormat(colour)
            .depthFormat(depth);
        declare(&cel, uniforms_->layout(), textures_->layout(), scenes_->layout());
        (skinned ? skinnedCel_ : cel_) = resources_->add(cel.build(cache_));

        // the back of a slightly larger hull, so the front faces are the ones culled
        const std::vector<uint32_t>& outlineVertex = skinned ? shaders.skinnedOutline : shaders.outlineVertex;
        pipeline::Builder outline(device_);
        outline.name(skinned ? "lit-outline-skinned" : "lit-outline")
            .shader(VK_SHADER_STAGE_VERTEX_BIT, outlineVertex.data(), bytes(outlineVertex))
            .shader(VK_SHADER_STAGE_FRAGMENT_BIT, shaders.outlineFragment.data(), bytes(shaders.outlineFragment));
        vertex(&outline, skinned, false);
        outline.cull(VK_CULL_MODE_FRONT_BIT, front)
            .depth(true, true)
            .blend(false)
            .colourFormat(colour)
            .depthFormat(depth);
        declare(&outline, uniforms_->layout(), textures_->layout(), scenes_->layout());
        (skinned ? skinnedOutline_ : outline_) = resources_->add(outline.build(cache_));

        if (shadow == VK_FORMAT_UNDEFINED) {
            continue;
        }
        // depth alone, so no fragment stage and no colour. Culled as the cel pass is, which
        // lighting::shadowLight keeps right by winding a face the way a camera does
        const std::vector<uint32_t>& shadowVertex = skinned ? shaders.skinnedShadow : shaders.shadow;
        pipeline::Builder caster(device_);
        caster.name(skinned ? "lit-shadow-skinned" : "lit-shadow")
            .shader(VK_SHADER_STAGE_VERTEX_BIT, shadowVertex.data(), bytes(shadowVertex));
        vertex(&caster, skinned, true);
        caster.cull(VK_CULL_MODE_BACK_BIT, front)
            .depth(true, true)
            .depthBias(true)
            .colourFormats({})
            .depthFormat(shadow);
        declare(&caster, uniforms_->layout(), textures_->layout(), scenes_->layout());
        (skinned ? skinnedShadow_ : shadow_) = resources_->add(caster.build(cache_));
    }
}

/**
 **/
PipelineHandle Lit::cel() const noexcept {
    return cel_;
}

/**
 **/
PipelineHandle Lit::outline() const noexcept {
    return outline_;
}

/**
 **/
PipelineHandle Lit::shadow() const noexcept {
    return shadow_;
}

/**
 **/
PipelineHandle Lit::skinnedCel() const noexcept {
    return skinnedCel_;
}

/**
 **/
PipelineHandle Lit::skinnedOutline() const noexcept {
    return skinnedOutline_;
}

/**
 **/
PipelineHandle Lit::skinnedShadow() const noexcept {
    return skinnedShadow_;
}

/**
 **/
VkDescriptorSetLayout Lit::sceneLayout() const noexcept {
    return scenes_->layout();
}

/**
 **/
Lit::Slot& Lit::slot() {
    Slot& slot = slots_[ring_->frame() % slots_.size()];
    if (slot.set != VK_NULL_HANDLE) {
        return slot;
    }

    slot.buffer = boost::make_shared<memory::Buffer>(device_, memory::BufferSpec{.usage = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, .bytes = sizeof(lighting::SceneUniforms)});
    slot.set = scenes_->allocate();

    VkDescriptorBufferInfo buffer{};
    buffer.buffer = slot.buffer->handle();
    buffer.range = sizeof(lighting::SceneUniforms);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = slot.set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer;
    // the set points at the buffer for good - a frame writes the buffer, not the descriptor
    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);
    reserve(&slot, &slot.palette, {.binding = 2, .needed = initialJoints * sizeof(glm::mat4), .kept = 0});
    reserve(&slot, &slot.objects, {.binding = 3, .needed = initialObjects * sizeof(Object), .kept = 0});
    return slot;
}

/**
 **/
void Lit::reserve(Slot* slot, boost::shared_ptr<memory::Buffer>* buffer, const Reservation& reservation) {
    const uint32_t binding = reservation.binding;
    const VkDeviceSize needed = reservation.needed;
    const VkDeviceSize kept = reservation.kept;
    if (*buffer && (*buffer)->size() >= needed) {
        return;
    }
    VkDeviceSize size = *buffer ? (*buffer)->size() : sizeof(glm::mat4);
    while (size < needed) {
        size *= 2;
    }
    const boost::shared_ptr<memory::Buffer> grown = boost::make_shared<memory::Buffer>(device_, memory::BufferSpec{.usage = VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, .bytes = size});
    if (*buffer) {
        if (kept > 0) {
            std::vector<unsigned char> bytes(static_cast<std::size_t>(kept));
            (*buffer)->read(bytes.data(), kept);
            grown->write(bytes.data(), kept);
        }
        // the frame that last read it should have finished, since the slot is only written
        // after waiting for it. It is still released through the ring, which destroys it only
        // once no frame in flight can read it
        ring_->retire([old = *buffer]() mutable { old.reset(); });
    }
    *buffer = grown;

    VkDescriptorBufferInfo info{};
    info.buffer = grown->handle();
    info.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = slot->set;
    write.dstBinding = binding;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &info;
    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);
}

/**
 **/
VkDescriptorSet Lit::scene(const lighting::SceneUniforms& uniforms, const TextureHandle& shadowMap, const std::vector<glm::mat4>& palette) {
    // the slot about to be written was last read by the frame that used it a ring ago
    ring_->waitFrame();
    Slot& written = slot();
    current_ = &written;
    written.count = 0;
    written.buffer->write(&uniforms, sizeof(uniforms));
    if (!palette.empty()) {
        const VkDeviceSize bytes = static_cast<VkDeviceSize>(palette.size() * sizeof(glm::mat4));
        reserve(&written, &written.palette, {.binding = 2, .needed = bytes, .kept = 0});
        written.palette->write(palette.data(), bytes);
    }

    const pipeline::Texture* texture = resources_->texture(shadowMap);
    if (texture == nullptr || !texture->image) {
        // white reads as the far plane, so nothing is in shadow
        texture = resources_->texture(textures_->white());
    }
    const VkDescriptorImageInfo image = texture->descriptor();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = written.set;
    write.dstBinding = 1;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;
    // written every frame rather than compared with last frame's, because a view destroyed and
    // another created can share a handle while the descriptor still names the one destroyed
    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);
    return written.set;
}

/**
 **/
uint32_t Lit::object(const Object& object) {
    if (current_ == nullptr) {
        throw std::runtime_error("Lit::object before any scene: a frame writes its scene first");
    }
    const VkDeviceSize offset = static_cast<VkDeviceSize>(current_->count) * sizeof(Object);
    // growing keeps what the frame has added so far, which its draws already name by index
    reserve(current_, &current_->objects, {.binding = 3, .needed = offset + sizeof(Object), .kept = offset});
    current_->objects->write(&object, sizeof(Object), offset);
    return current_->count++;
}

};  // namespace v3d::render::realtime::vulkan::renderer
