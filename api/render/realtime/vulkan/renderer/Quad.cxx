/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Quad.h"

#include <api/render/realtime/DrawItem.h>
#include <api/render/realtime/vulkan/device/Result.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/pipeline/Builder.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <sstream>
#include <stdexcept>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan::renderer {

namespace {

/**
 * The quad pipeline's shader modules, compiled to SPIR-V at build time by glslc and
 * included here as the C initialiser lists its -mfmt=c writes - see v3d_add_shader in
 * the root CMakeLists.
 **/
const uint32_t vertexShader[] =
#include "shaders/quad.vert.inc"
;  // NOLINT(whitespace/semicolon)

const uint32_t fragmentShader[] =
#include "shaders/quad.frag.inc"
;  // NOLINT(whitespace/semicolon)

/**
 * How many sets a descriptor pool is created with. One set per texture; another pool
 * is added when this one is full.
 **/
const uint32_t poolSize = 64;

/**
 * What each geometry buffer starts at, in bytes. A screen of quads fits without
 * growing, and the buffers double from here when something does not.
 **/
const VkDeviceSize initialVertexBytes = 64ULL * 1024;
const VkDeviceSize initialIndexBytes = 32ULL * 1024;

/**
 * The push constant block, laid out as quad.vert and quad.frag declare it.
 *
 * One range covers both stages: the projection is the vertex stage's and text is the
 * fragment stage's, and a batch is one or the other for all of its fragments.
 **/
struct Push final {
    glm::mat4 projection;
    uint32_t text;
};

};  // namespace

/**
 **/
Quad::Quad(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<device::Device>& device,
    const boost::shared_ptr<pipeline::Cache>& cache, const boost::shared_ptr<pipeline::Resources>& resources,
    const boost::shared_ptr<frame::Ring>& ring, const boost::shared_ptr<frame::FrameUniforms>& uniforms,
    VkFormat colour, VkFormat depth) :
    logger_(logger),
    device_(device),
    cache_(cache),
    resources_(resources),
    ring_(ring),
    uniforms_(uniforms),
    cursor_(0) {
    factory_ = boost::make_shared<memory::TextureFactory>(device_);
    createLayouts();
    createPipelines(colour, depth);
    geometry_.resize(ring_->framesInFlight() > 0 ? ring_->framesInFlight() : 1);
    createWhite();
}

/**
 **/
Quad::~Quad() {
    // the pipelines, their layouts, the textures and the materials belong to pipeline::Resources -
    // what is owned here is the descriptor pool and the geometry buffers, which go with it
}

/**
 **/
void Quad::createLayouts() {
    VkDescriptorSetLayoutBinding sampler{};
    sampler.binding = 0;
    sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    sampler.descriptorCount = 1;
    sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

    materialSets_ = boost::make_shared<pipeline::DescriptorPool>(device_, ring_,
        std::vector<VkDescriptorSetLayoutBinding>{sampler}, poolSize, "per material");
}

/**
 **/
void Quad::createPipelines(VkFormat colour, VkFormat depth) {
    pipeline::Builder builder(device_);
    builder.name("quad")
        .shader(VK_SHADER_STAGE_VERTEX_BIT, vertexShader, sizeof(vertexShader))
        .shader(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader, sizeof(fragmentShader))
        .vertexBinding(0, sizeof(Canvas::Vertex))
        .vertexAttribute(0, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Canvas::Vertex, position))
        .vertexAttribute(1, 0, VK_FORMAT_R32G32_SFLOAT, offsetof(Canvas::Vertex, uv))
        .vertexAttribute(2, 0, VK_FORMAT_R32G32B32A32_SFLOAT, offsetof(Canvas::Vertex, colour))
        // nothing 2D has a back face worth culling, and not culling means a caller cannot
        // get a quad's winding wrong and have it silently disappear
        .cull(VK_CULL_MODE_NONE)
        .set(uniforms_->layout())
        .set(materialSets_->layout())
        .push(VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT, sizeof(Push))
        .colourFormat(colour);

    pipeline_ = resources_->add(builder.build(cache_));

    builder.name("quad-depth").depthFormat(depth);
    depthPipeline_ = resources_->add(builder.build(cache_));
}

/**
 **/
Quad::Geometry Quad::claim() {
    std::vector<Geometry>& ring = geometry_[ring_->frame()];
    if (cursor_ >= ring.size()) {
        Geometry geometry;
        geometry.vertices = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, initialVertexBytes);
        geometry.indices = boost::make_shared<memory::Buffer>(device_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, initialIndexBytes);
        ring.push_back(geometry);
    }
    return ring[cursor_++];
}

/**
 **/
void Quad::endFrame() noexcept {
    cursor_ = 0;
}

/**
 **/
VkDescriptorSetLayout Quad::materialLayout() const noexcept {
    return materialSets_->layout();
}

/**
 **/
void Quad::createWhite() {
    const unsigned char pixel[4] = {0xFF, 0xFF, 0xFF, 0xFF};
    white_ = texture(pixel, 1, 1, 4);
}

/**
 **/
TextureHandle Quad::texture(const boost::shared_ptr<v3d::image::Image>& image, memory::TextureFactory::Encoding encoding) {
    return resources_->add(factory_->create(image, encoding));
}

/**
 **/
TextureHandle Quad::texture(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels) {
    return resources_->add(factory_->create(pixels, width, height, channels));
}

/**
 **/
TextureHandle Quad::texture(const frame::RenderTarget& target, uint32_t slot) {
    // a depth-only target has no colour to read, and a set written against no image is a
    // validation error - the same answer depthTexture() gives a target with no depth to read
    if (target.view() == VK_NULL_HANDLE || slot >= target.images()) {
        return white_;
    }
    return resources_->add(target.texture(slot));
}

/**
 **/
TextureHandle Quad::depthTexture(const frame::RenderTarget& target, uint32_t slot) {
    if (!target.sampledDepth() || slot >= target.images()) {
        return white_;
    }
    return resources_->add(target.depthTexture(slot));
}

/**
 **/
TextureHandle Quad::white() const noexcept {
    return white_;
}

/**
 **/
MaterialHandle Quad::material(const TextureHandle& handle) {
    const std::map<TextureHandle, MaterialHandle>::const_iterator found = materials_.find(handle);
    if (found != materials_.end()) {
        return found->second;
    }

    const pipeline::Texture* texture = resources_->texture(handle);
    if (texture == nullptr) {
        return MaterialHandle();
    }

    // a set that was released before is written again here, so every write is a full one
    VkDescriptorSet set = materialSets_->allocate();

    VkDescriptorImageInfo image{};
    image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
    image.imageView = texture->image->view();
    image.sampler = texture->sampler->handle();

    VkWriteDescriptorSet write{};
    write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
    write.dstSet = set;
    write.dstBinding = 0;
    write.descriptorCount = 1;
    write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
    write.pImageInfo = &image;

    vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);

    pipeline::Material built;
    built.set = set;
    built.texture = handle;

    const MaterialHandle material = resources_->add(built);
    materials_[handle] = material;
    return material;
}

/**
 **/
bool Quad::release(const TextureHandle& handle) {
    if (handle == white_) {
        return false;
    }

    const std::map<TextureHandle, MaterialHandle>::const_iterator found = materials_.find(handle);
    if (found != materials_.end()) {
        const pipeline::Material* material = resources_->material(found->second);
        if (material != nullptr) {
            materialSets_->release(material->set);
        }
        resources_->release(found->second);
        materials_.erase(found);
    }

    return resources_->release(handle);
}

/**
 **/
void Quad::submit(const Canvas& canvas, Pass* pass, uint16_t layer) {
    if (pass == nullptr || canvas.empty()) {
        return;
    }

    // the device may still be reading what this frame's slots held two frames ago
    ring_->waitFrame();

    const Geometry claimed = claim();
    const boost::shared_ptr<memory::Buffer>& vertices = claimed.vertices;
    const boost::shared_ptr<memory::Buffer>& indices = claimed.indices;

    const VkDeviceSize vertexBytes = canvas.vertices().size() * sizeof(Canvas::Vertex);
    const VkDeviceSize indexBytes = canvas.indices().size() * sizeof(uint32_t);

    vertices->grow(vertexBytes);
    indices->grow(indexBytes);

    vertices->write(canvas.vertices().data(), vertexBytes);
    indices->write(canvas.indices().data(), indexBytes);

    const glm::mat4 projection = canvas.projection();

    // dynamic rendering matches a pipeline to the pass's attachments, so which of the two
    // is drawn with follows from whether the pass has a depth buffer
    const PipelineHandle handle = pass->depth() ? depthPipeline_ : pipeline_;
    const pipeline::Pipeline* pipeline = resources_->pipeline(handle);
    for (const Canvas::Batch& batch : canvas.batches()) {
        if (batch.indices == 0) {
            continue;
        }
        // an unset texture is the untextured case, drawn against white
        const MaterialHandle bound = material(batch.texture.valid() ? batch.texture : white_);

        DrawItem item;
        item.key.layer = layer;
        item.key.pipeline = static_cast<uint16_t>(handle.id());
        item.key.material = static_cast<uint16_t>(bound.id());
        item.pipeline = handle;
        item.material = bound;
        item.vertexBuffer = vertices->handle();
        item.indexBuffer = indices->handle();
        item.indexType = VK_INDEX_TYPE_UINT32;
        item.indices = batch.indices;
        item.firstIndex = batch.firstIndex;
        item.instances = 1;

        if (batch.clipped) {
            // the canvas clips in its own pixels, which are the image's because the ui is
            // drawn into a pass covering the whole of it - ADR-0037
            const float left = std::max(batch.clip.x, 0.0f);
            const float top = std::max(batch.clip.y, 0.0f);
            item.scissored = true;
            item.scissor.offset.x = static_cast<int32_t>(left);
            item.scissor.offset.y = static_cast<int32_t>(top);
            item.scissor.extent.width = static_cast<uint32_t>(std::max(batch.clip.z - left, 0.0f));
            item.scissor.extent.height = static_cast<uint32_t>(std::max(batch.clip.w - top, 0.0f));
        }

        if (pipeline != nullptr && pipeline->pushStages != 0) {
            Push constants;
            constants.projection = projection;
            constants.text = batch.text ? 1u : 0u;
            std::memcpy(item.push.data(), &constants, sizeof(constants));
            item.pushSize = sizeof(constants);
        }

        pass->submit(item);
    }
}

};  // namespace v3d::render::realtime::vulkan::renderer
