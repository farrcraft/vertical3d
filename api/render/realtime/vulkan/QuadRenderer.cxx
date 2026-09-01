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

        /**
         **/
        VkShaderModule createModule(VkDevice device, const uint32_t* code, std::size_t bytes) {
            VkShaderModuleCreateInfo info{};
            info.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            info.codeSize = bytes;
            info.pCode = code;

            VkShaderModule module = VK_NULL_HANDLE;
            VkResult result = vkCreateShaderModule(device, &info, nullptr, &module);
            if (result != VK_SUCCESS) {
                std::stringstream msg;
                msg << "Unable to create a vulkan shader module - " << resultString(result);
                throw std::runtime_error(msg.str());
            }
            return module;
        }

    };  // namespace

    /**
     **/
    QuadRenderer::QuadRenderer(const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<Device>& device,
        const boost::shared_ptr<PipelineCache>& cache, const boost::shared_ptr<Resources>& resources,
        const boost::shared_ptr<Presenter>& presenter, VkFormat colour) :
        logger_(logger),
        device_(device),
        cache_(cache),
        resources_(resources),
        presenter_(presenter),
        frameLayout_(VK_NULL_HANDLE),
        materialLayout_(VK_NULL_HANDLE),
        remaining_(0) {
        factory_ = boost::make_shared<TextureFactory>(device_);
        createLayouts();
        createPipeline(colour);
        createBuffers();
        createWhite();
    }

    /**
     **/
    QuadRenderer::~QuadRenderer() {
        // the pipeline, its layout, the textures and the materials all belong to Resources -
        // what is owned here is the descriptor machinery and the geometry buffers
        for (VkDescriptorPool pool : pools_) {
            vkDestroyDescriptorPool(device_->handle(), pool, nullptr);
        }
        pools_.clear();

        if (materialLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_->handle(), materialLayout_, nullptr);
        }
        if (frameLayout_ != VK_NULL_HANDLE) {
            vkDestroyDescriptorSetLayout(device_->handle(), frameLayout_, nullptr);
        }
    }

    /**
     **/
    void QuadRenderer::createLayouts() {
        // set 0 is the per frame frequency of the convention in docs/RenderingPipeline.md.
        // Nothing binds anything at it - the projection is a push constant - but the layout
        // has to exist for the sampler to sit at set 1
        VkDescriptorSetLayoutCreateInfo frame{};
        frame.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        frame.bindingCount = 0;

        VkResult result = vkCreateDescriptorSetLayout(device_->handle(), &frame, nullptr, &frameLayout_);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create the per frame descriptor set layout - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        VkDescriptorSetLayoutBinding sampler{};
        sampler.binding = 0;
        sampler.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        sampler.descriptorCount = 1;
        sampler.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        VkDescriptorSetLayoutCreateInfo material{};
        material.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        material.bindingCount = 1;
        material.pBindings = &sampler;

        result = vkCreateDescriptorSetLayout(device_->handle(), &material, nullptr, &materialLayout_);
        if (result != VK_SUCCESS) {
            std::stringstream msg;
            msg << "Unable to create the per material descriptor set layout - " << resultString(result);
            throw std::runtime_error(msg.str());
        }
    }

    /**
     **/
    void QuadRenderer::createPipeline(VkFormat colour) {
        VkDevice device = device_->handle();

        VkShaderModule vertex = createModule(device, vertexShader, sizeof(vertexShader));
        VkShaderModule fragment = VK_NULL_HANDLE;
        try {
            fragment = createModule(device, fragmentShader, sizeof(fragmentShader));
        } catch (...) {
            vkDestroyShaderModule(device, vertex, nullptr);
            throw;
        }

        Pipeline built;
        built.pushStages = VK_SHADER_STAGE_VERTEX_BIT;

        VkPushConstantRange push{};
        push.stageFlags = built.pushStages;
        push.offset = 0;
        push.size = sizeof(glm::mat4);

        const VkDescriptorSetLayout sets[2] = {frameLayout_, materialLayout_};

        VkPipelineLayoutCreateInfo layout{};
        layout.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        layout.setLayoutCount = 2;
        layout.pSetLayouts = sets;
        layout.pushConstantRangeCount = 1;
        layout.pPushConstantRanges = &push;

        VkResult result = vkCreatePipelineLayout(device, &layout, nullptr, &built.layout);
        if (result != VK_SUCCESS) {
            vkDestroyShaderModule(device, fragment, nullptr);
            vkDestroyShaderModule(device, vertex, nullptr);
            std::stringstream msg;
            msg << "Unable to create the quad pipeline layout - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        VkPipelineShaderStageCreateInfo stages[2]{};
        stages[0].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[0].stage = VK_SHADER_STAGE_VERTEX_BIT;
        stages[0].module = vertex;
        stages[0].pName = "main";
        stages[1].sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        stages[1].stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        stages[1].module = fragment;
        stages[1].pName = "main";

        VkVertexInputBindingDescription binding{};
        binding.binding = 0;
        binding.stride = sizeof(Canvas::Vertex);
        binding.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        VkVertexInputAttributeDescription attributes[3]{};
        attributes[0].location = 0;
        attributes[0].binding = 0;
        attributes[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[0].offset = offsetof(Canvas::Vertex, position);
        attributes[1].location = 1;
        attributes[1].binding = 0;
        attributes[1].format = VK_FORMAT_R32G32_SFLOAT;
        attributes[1].offset = offsetof(Canvas::Vertex, uv);
        attributes[2].location = 2;
        attributes[2].binding = 0;
        attributes[2].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributes[2].offset = offsetof(Canvas::Vertex, colour);

        VkPipelineVertexInputStateCreateInfo input{};
        input.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        input.vertexBindingDescriptionCount = 1;
        input.pVertexBindingDescriptions = &binding;
        input.vertexAttributeDescriptionCount = 3;
        input.pVertexAttributeDescriptions = attributes;

        VkPipelineInputAssemblyStateCreateInfo assembly{};
        assembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        assembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;

        VkPipelineViewportStateCreateInfo viewport{};
        viewport.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewport.viewportCount = 1;
        viewport.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo raster{};
        raster.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        raster.polygonMode = VK_POLYGON_MODE_FILL;
        // nothing 2D has a back face worth culling, and not culling means a caller cannot get
        // a quad's winding wrong and have it silently disappear
        raster.cullMode = VK_CULL_MODE_NONE;
        raster.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        raster.lineWidth = 1.0f;

        VkPipelineMultisampleStateCreateInfo multisample{};
        multisample.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisample.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineDepthStencilStateCreateInfo depth{};
        depth.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        // 2D content is painter ordered, so it neither tests nor writes depth
        depth.depthTestEnable = VK_FALSE;
        depth.depthWriteEnable = VK_FALSE;

        VkPipelineColorBlendAttachmentState blend{};
        blend.blendEnable = VK_TRUE;
        blend.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        blend.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.colorBlendOp = VK_BLEND_OP_ADD;
        blend.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        blend.dstAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        blend.alphaBlendOp = VK_BLEND_OP_ADD;
        blend.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;

        VkPipelineColorBlendStateCreateInfo blending{};
        blending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        blending.attachmentCount = 1;
        blending.pAttachments = &blend;

        // the viewport is dynamic so that a window resize costs no pipeline rebuild
        const VkDynamicState dynamics[2] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};
        VkPipelineDynamicStateCreateInfo dynamic{};
        dynamic.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamic.dynamicStateCount = 2;
        dynamic.pDynamicStates = dynamics;

        // dynamic rendering, so the formats come from here rather than from a render pass
        VkPipelineRenderingCreateInfo rendering{};
        rendering.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;
        rendering.colorAttachmentCount = 1;
        rendering.pColorAttachmentFormats = &colour;

        VkGraphicsPipelineCreateInfo info{};
        info.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        info.pNext = &rendering;
        info.stageCount = 2;
        info.pStages = stages;
        info.pVertexInputState = &input;
        info.pInputAssemblyState = &assembly;
        info.pViewportState = &viewport;
        info.pRasterizationState = &raster;
        info.pMultisampleState = &multisample;
        info.pDepthStencilState = &depth;
        info.pColorBlendState = &blending;
        info.pDynamicState = &dynamic;
        info.layout = built.layout;

        result = vkCreateGraphicsPipelines(device, cache_->handle(), 1, &info, nullptr, &built.pipeline);

        // the modules are only needed while the pipeline is being compiled
        vkDestroyShaderModule(device, fragment, nullptr);
        vkDestroyShaderModule(device, vertex, nullptr);

        if (result != VK_SUCCESS) {
            vkDestroyPipelineLayout(device, built.layout, nullptr);
            std::stringstream msg;
            msg << "Unable to create the quad pipeline - " << resultString(result);
            throw std::runtime_error(msg.str());
        }

        pipeline_ = resources_->add(built);
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

        const Pipeline* pipeline = resources_->pipeline(pipeline_);
        for (const Canvas::Batch& batch : canvas.batches()) {
            if (batch.indices == 0) {
                continue;
            }
            // an unset texture is the untextured case, drawn against white
            const MaterialHandle bound = material(batch.texture.valid() ? batch.texture : white_);

            DrawItem item;
            item.key.layer = layer;
            item.key.pipeline = static_cast<uint16_t>(pipeline_.id());
            item.key.material = static_cast<uint16_t>(bound.id());
            item.pipeline = pipeline_;
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
