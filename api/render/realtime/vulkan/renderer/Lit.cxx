/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lit.h"

#include <api/render/realtime/vulkan/pipeline/Builder.h>
#include <api/type/Model.h>

#include <cstddef>
#include <iterator>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::renderer {

namespace {

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
 * Sets 0 to 2 and the push block, which every lit pipeline declares alike so that the sets a
 * pass binds stay bound across a switch between them.
 **/
void declare(pipeline::Builder* builder, VkDescriptorSetLayout camera, VkDescriptorSetLayout material,
    VkDescriptorSetLayout scene) {
    builder->set(camera)
        .set(material)
        .set(scene)
        .push(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Lit::Object));
}

/**
 * The vertex a pipeline reads: a model's, and for a skinned one its influence after it - the
 * layout MeshRegistry uploads a skinned model in. A depth only stage reads the position alone.
 **/
void vertex(pipeline::Builder* builder, bool skinned, bool positionOnly) {
    using Vertex = type::Model::Vertex;
    using Influence = type::Model::Influence;
    builder->vertexBinding(0, skinned ? sizeof(Vertex) + sizeof(Influence) : sizeof(Vertex))
        .vertexAttribute(0, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, position));
    if (!positionOnly) {
        builder->vertexAttribute(1, 0, VK_FORMAT_R32G32B32_SFLOAT, offsetof(Vertex, normal))
            .vertexAttribute(2, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Vertex, uv));
    }
    if (skinned) {
        builder->vertexAttribute(3, 0, VK_FORMAT_R16G16B16A16_UINT, sizeof(Vertex) + offsetof(Influence, joints))
            .vertexAttribute(4, 0, VK_FORMAT_R32G32B32A32_SFLOAT, sizeof(Vertex) + offsetof(Influence, weights));
    }
}

/**
 * The palette's starting room, in matrices: enough for a handful of characters before the
 * first grow.
 **/
const std::size_t initialJoints = 256;

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
    VkFormat colour, VkFormat depth, VkFormat shadow, const Shaders& shaders) :
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

    // a set per frame in flight, which is all a scene ever asks for
    scenes_ = boost::make_shared<pipeline::DescriptorPool>(device_, ring_,
        std::vector<VkDescriptorSetLayoutBinding>{block, map, palette}, ring_->framesInFlight(), "scene");
    slots_.resize(ring_->framesInFlight() > 0 ? ring_->framesInFlight() : 1);

    createPipelines(shaders, colour, depth, shadow);
}

/**
 **/
void Lit::createPipelines(const Shaders& shaders, VkFormat colour, VkFormat depth, VkFormat shadow) {
    const auto bytes = [](const std::vector<uint32_t>& code) { return code.size() * sizeof(uint32_t); };

    for (const bool skinned : {false, true}) {
        const std::vector<uint32_t>& meshVertex = skinned ? shaders.skinnedMesh : shaders.mesh;
        pipeline::Builder cel(device_);
        cel.name(skinned ? "lit-cel-skinned" : "lit-cel")
            .shader(VK_SHADER_STAGE_VERTEX_BIT, meshVertex.data(), bytes(meshVertex))
            .shader(VK_SHADER_STAGE_FRAGMENT_BIT, shaders.cel.data(), bytes(shaders.cel));
        vertex(&cel, skinned, false);
        cel.cull(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
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
        outline.cull(VK_CULL_MODE_FRONT_BIT, VK_FRONT_FACE_CLOCKWISE)
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
        // shadow::light keeps right by winding a face the way a camera does
        const std::vector<uint32_t>& shadowVertex = skinned ? shaders.skinnedShadow : shaders.shadow;
        pipeline::Builder caster(device_);
        caster.name(skinned ? "lit-shadow-skinned" : "lit-shadow")
            .shader(VK_SHADER_STAGE_VERTEX_BIT, shadowVertex.data(), bytes(shadowVertex));
        vertex(&caster, skinned, true);
        caster.cull(VK_CULL_MODE_BACK_BIT, VK_FRONT_FACE_CLOCKWISE)
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

    slot.buffer = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, sizeof(SceneUniforms));
    slot.set = scenes_->allocate();

    VkDescriptorBufferInfo buffer{};
    buffer.buffer = slot.buffer->handle();
    buffer.range = sizeof(SceneUniforms);

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = slot.set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
    write.pBufferInfo = &buffer;
    // the set points at the buffer for good - a frame writes the buffer, not the descriptor
    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);
    reserve(&slot, initialJoints);
    return slot;
}

/**
 **/
void Lit::reserve(Slot* slot, std::size_t joints) {
    const VkDeviceSize needed = static_cast<VkDeviceSize>(joints) * sizeof(glm::mat4);
    if (slot->palette && slot->palette->size() >= needed) {
        return;
    }
    VkDeviceSize size = slot->palette ? slot->palette->size() : sizeof(glm::mat4);
    while (size < needed) {
        size *= 2;
    }
    if (slot->palette) {
        // the frame that last read it has finished, since the slot is only written after
        // waiting for it, but the ring is what is trusted with that - ADR-0061
        ring_->retire([old = slot->palette]() mutable { old.reset(); });
    }
    slot->palette = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT, size);

    VkDescriptorBufferInfo buffer{};
    buffer.buffer = slot->palette->handle();
    buffer.range = VK_WHOLE_SIZE;

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = slot->set;
    write.dstBinding = 2;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
    write.pBufferInfo = &buffer;
    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);
}

/**
 **/
VkDescriptorSet Lit::scene(const SceneUniforms& uniforms, const TextureHandle& shadowMap, const std::vector<glm::mat4>& palette) {
    // the slot about to be written was last read by the frame that used it a ring ago
    ring_->waitFrame();
    Slot& written = slot();
    written.buffer->write(&uniforms, sizeof(uniforms));
    if (!palette.empty()) {
        reserve(&written, palette.size());
        written.palette->write(palette.data(), static_cast<VkDeviceSize>(palette.size() * sizeof(glm::mat4)));
    }

    const pipeline::Texture* texture = resources_->texture(shadowMap);
    if (texture == nullptr || !texture->image) {
        // white reads as the far plane, so nothing is in shadow
        texture = resources_->texture(textures_->white());
    }
    VkDescriptorImageInfo image{};
    // a depth image is left read only for depth by the recorder - ADR-0044 - and a colour one
    // for sampling
    image.imageLayout = (texture->image->spec().aspect & VK_IMAGE_ASPECT_DEPTH_BIT) != 0
        ? VK_IMAGE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL
        : VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image.imageView = texture->image->view();
    image.sampler = texture->sampler->handle();

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

};  // namespace v3d::render::realtime::vulkan::renderer
