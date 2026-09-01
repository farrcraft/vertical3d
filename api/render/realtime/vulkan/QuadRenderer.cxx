/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "QuadRenderer.h"

#include <cstddef>
#include <cstring>
#include <map>
#include <sstream>
#include <stdexcept>

#include "PipelineBuilder.h"
#include "Result.h"

#include "../DrawItem.h"

#include <boost/make_shared.hpp>

namespace v3d::render::realtime::vulkan {

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
        const VkDeviceSize initialVertexBytes = 64 * 1024;
        const VkDeviceSize initialIndexBytes = 32 * 1024;

    };  // namespace

    /**
     **/
    QuadRenderer::QuadRenderer(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Device>& device,
        const boost::shared_ptr<PipelineCache>& cache, const boost::shared_ptr<Resources>& resources,
        const boost::shared_ptr<Presenter>& presenter, const boost::shared_ptr<FrameUniforms>& uniforms,
        VkFormat colour, VkFormat depth) :
        logger_(logger),
        device_(device),
        cache_(cache),
        resources_(resources),
        presenter_(presenter),
        uniforms_(uniforms),
        materialLayout_(VK_NULL_HANDLE),
        remaining_(0) {
        factory_ = boost::make_shared<TextureFactory>(device_);
        createLayouts();
        createPipelines(colour, depth);
        createBuffers();
        createWhite();
    }

    /**
     **/
    QuadRenderer::~QuadRenderer() {
        // the pipelines, their layouts, the textures and the materials belong to Resources -
        // what is owned here is the descriptor machinery and the geometry buffers
        for (VkDescriptorPool pool : pools_) {
            vkDestroyDescriptorPool(device_->handle(), pool, nullptr);
        }
        pools_.clear();

        if (materialLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_->handle(), materialLayout_, nullptr);
        }
    }

    /**
     **/
    void QuadRenderer::createLayouts() {
        VkDescriptorSetLayoutBinding sampler{};
        sampler.binding = 0;
        sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler.descriptorCount = 1;
        sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo material{};
        material.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        material.bindingCount = 1;
        material.pBindings = &sampler;

        VkResult result = vkCreateDescriptorSetLayout(device_->handle(), &material, nullptr, &materialLayout_);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create the per material descriptor set layout - " << resultString(result);
            throw std::runtime_error(msg.str());
        }
    }

    /**
     **/
    void QuadRenderer::createPipelines(VkFormat colour, VkFormat depth) {
        PipelineBuilder builder(device_);
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
            .set(materialLayout_)
            .push(VK_SHADER_STAGE_VERTEX_BIT, sizeof(glm::mat4))
            .colourFormat(colour);

        pipeline_ = resources_->add(builder.build(cache_));

        builder.name("quad-depth").depthFormat(depth);
        depthPipeline_ = resources_->add(builder.build(cache_));
    }

    /**
     **/
    void QuadRenderer::createBuffers() {
        for (uint32_t frame = 0; frame < presenter_->framesInFlight(); frame++) {
            vertices_.push_back(boost::make_shared<Buffer>(device_, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT, initialVertexBytes));
            indices_.push_back(boost::make_shared<Buffer>(device_, VK_BUFFER_USAGE_INDEX_BUFFER_BIT, initialIndexBytes));
        }
    }

    /**
     **/
    void QuadRenderer::createWhite() {
        const unsigned char pixel[4] = {0xFF, 0xFF, 0xFF, 0xFF};
        white_ = texture(pixel, 1, 1, 4);
    }

    /**
     **/
    TextureHandle QuadRenderer::texture(const boost::shared_ptr<v3d::image::Image>& image) {
        return resources_->add(factory_->create(image));
    }

    /**
     **/
    TextureHandle QuadRenderer::texture(const unsigned char* pixels, uint32_t width, uint32_t height, uint32_t channels) {
        return resources_->add(factory_->create(pixels, width, height, channels));
    }

    /**
     **/
    TextureHandle QuadRenderer::white() const noexcept {
        return white_;
    }

    /**
     **/
    void QuadRenderer::addPool() {
        VkDescriptorPoolSize size{};
        size.type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        size.descriptorCount = poolSize;

        VkDescriptorPoolCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        info.maxSets = poolSize;
        info.poolSizeCount = 1;
        info.pPoolSizes = &size;

        VkDescriptorPool pool = VK_NULL_HANDLE;
        VkResult result = vkCreateDescriptorPool(device_->handle(), &info, nullptr, &pool);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create a vulkan descriptor pool - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        pools_.push_back(pool);
        remaining_ = poolSize;
    }

    /**
     **/
    MaterialHandle QuadRenderer::material(const TextureHandle& handle) {
        const std::map<uint32_t, MaterialHandle>::const_iterator found = materials_.find(handle.id());
        if (found != materials_.end()) {
            return found->second;
        }

        const Texture* texture = resources_->texture(handle);
        if (texture == nullptr) {
            return MaterialHandle();
        }

        if (pools_.empty() || remaining_ == 0) {
            addPool();
        }

        VkDescriptorSetAllocateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        info.descriptorPool = pools_.back();
        info.descriptorSetCount = 1;
        info.pSetLayouts = &materialLayout_;

        VkDescriptorSet set = VK_NULL_HANDLE;
        VkResult result = vkAllocateDescriptorSets(device_->handle(), &info, &set);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to allocate a vulkan descriptor set - " << resultString(result);
            throw std::runtime_error(msg.str());
        }
        remaining_--;

        VkDescriptorImageInfo image{};
        image.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        image.imageView = texture->view;
        image.sampler = texture->sampler;

        VkWriteDescriptorSet write{};
        write.sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
        write.dstSet = set;
        write.dstBinding = 0;
        write.descriptorCount = 1;
        write.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        write.pImageInfo = &image;

        vkUpdateDescriptorSets(device_->handle(), 1, &write, 0, nullptr);

        Material built;
        built.set = set;
        built.texture = handle;

        const MaterialHandle material = resources_->add(built);
        materials_[handle.id()] = material;
        return material;
    }

    /**
     **/
    void QuadRenderer::submit(const Canvas& canvas, Pass* pass, uint16_t layer) {
        if (pass == nullptr || canvas.empty()) {
            return;
        }

        const uint32_t frame = presenter_->frame();
        // the device may still be reading what this slot held two frames ago
        presenter_->waitFrame();

        const boost::shared_ptr<Buffer>& vertices = vertices_[frame];
        const boost::shared_ptr<Buffer>& indices = indices_[frame];

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
        const Pipeline* pipeline = resources_->pipeline(handle);
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

            if (pipeline != nullptr && pipeline->pushStages != 0) {
                std::memcpy(item.push.data(), &projection, sizeof(projection));
                item.pushSize = sizeof(projection);
            }

            pass->submit(item);
        }
    }

};  // namespace v3d::render::realtime::vulkan
