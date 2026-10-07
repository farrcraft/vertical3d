/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "FullScreen.h"

#include <api/render/realtime/vulkan/pipeline/Builder.h>

#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::renderer {

namespace {

/**
 * The triangle every full-screen pipeline draws, compiled to SPIR-V at build time - see
 * v3d_add_shader.
 **/
const uint32_t vertexShader[] =
#include "shaders/fullscreen.vert.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * How many sources a pool holds before it grows another. A source is made when a target is
 * made or resized, so a handful covers most programs.
 **/
const uint32_t setsPerPool = 8;

};  // namespace

/**
 **/
FullScreen::FullScreen(const boost::shared_ptr<device::Device>& device, const boost::shared_ptr<pipeline::Cache>& cache,
    const boost::shared_ptr<pipeline::Resources>& resources, const boost::shared_ptr<frame::Ring>& ring,
    const boost::shared_ptr<frame::FrameUniforms>& uniforms, const Spec& spec) :
    device_(device),
    resources_(resources),
    count_(spec.sources) {
    if (count_ == 0) {
        std::stringstream msg;
        msg << "The " << spec.name << " full screen pipeline reads no source";
        throw std::runtime_error(msg.str());
    }

    std::vector<VkDescriptorSetLayoutBinding> bindings(count_);
    for (uint32_t binding = 0; binding < count_; binding++) {
        bindings[binding].binding = binding;
        bindings[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        bindings[binding].descriptorCount = 1;
        bindings[binding].stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;
    }
    sources_ = boost::make_shared<pipeline::DescriptorPool>(device_, ring, bindings, setsPerPool, spec.name);

    pipeline::Builder builder(device_);
    builder.name(spec.name)
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, spec.fragment.data(), spec.fragment.size() * sizeof(uint32_t))
        .cull(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE)
        .depth(false, false)
        // every pixel is written once, from what is read, so there is nothing under it to blend with
        .blend(false)
        .set(uniforms->layout())
        .set(sources_->layout())
        .colourFormat(spec.colour)
        .depthFormat(spec.depth);
    pipeline_ = resources_->add(builder.build(cache));
}

/**
 **/
MaterialHandle FullScreen::source(const std::vector<TextureHandle>& textures) {
    if (textures.size() != count_) {
        std::stringstream msg;
        msg << "A full screen source was given " << textures.size() << " images for " << count_ << " bindings";
        throw std::runtime_error(msg.str());
    }

    std::vector<VkDescriptorImageInfo> images(count_);
    for (uint32_t binding = 0; binding < count_; binding++) {
        const pipeline::Texture* texture = resources_->texture(textures[binding]);
        if (texture == nullptr || !texture->image || !texture->sampler) {
            throw std::runtime_error("A full screen source names a texture that has been released, or has no image");
        }
        images[binding] = texture->descriptor();
    }

    VkDescriptorSet set = sources_->allocate();
    std::vector<VkWriteDescriptorSet> writes(count_);
    for (uint32_t binding = 0; binding < count_; binding++) {
        writes[binding].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        writes[binding].dstSet = set;
        writes[binding].dstBinding = binding;
        writes[binding].descriptorCount = 1;
        writes[binding].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        writes[binding].pImageInfo = &images[binding];
    }
    vkUpdateDescriptorSets(device_->handle(), count_, writes.data(), 0, nullptr);

    pipeline::Material material;
    material.set = set;
    material.texture = textures.front();
    return resources_->add(material);
}

/**
 **/
bool FullScreen::release(const MaterialHandle& source) {
    const pipeline::Material* material = resources_->material(source);
    if (material == nullptr) {
        return false;
    }
    // a released material goes on resolving for the frame already queued, so the set is handed
    // back only by the release that took effect, never by a second one
    VkDescriptorSet set = material->set;
    if (!resources_->release(source)) {
        return false;
    }
    sources_->release(set);
    return true;
}

/**
 **/
DrawItem FullScreen::item(const MaterialHandle& source) const {
    DrawItem item;
    item.pipeline = pipeline_;
    item.material = source;
    // no vertex buffer: the vertex stage makes the triangle from its index
    item.vertices = 3;
    return item;
}

/**
 **/
void FullScreen::submit(const MaterialHandle& source, Pass* pass) const {
    pass->submit(item(source));
}

/**
 **/
PipelineHandle FullScreen::pipeline() const noexcept {
    return pipeline_;
}

};  // namespace v3d::render::realtime::vulkan::renderer
