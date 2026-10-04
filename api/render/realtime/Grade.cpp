/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Grade.h"

#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/memory/Buffer.h>
#include <api/render/realtime/vulkan/memory/Image.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>
#include <api/render/realtime/vulkan/renderer/FullScreen.h>

#include <cstddef>
#include <iterator>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

namespace {

const uint32_t gradeShader[] =
#include "shaders/grade.frag.inc"
;  // NOLINT(whitespace/semicolon)

const std::size_t TEXELS = static_cast<std::size_t>(Grade::SIZE) * Grade::SIZE * Grade::SIZE;

/**
 * Copy the table into a 3D image and leave it ready to sample.
 **/
void upload(const boost::shared_ptr<DeviceContext>& context, const vulkan::memory::Image& image,
    const std::vector<uint8_t>& texels) {
    vulkan::memory::Buffer staging(context->device(), VK_BUFFER_USAGE_TRANSFER_SRC_BIT, texels.size());
    staging.write(texels.data(), texels.size());

    VkImage target = image.handle();
    VkBuffer source = staging.handle();
    context->uploader()->oneShot([target, source](VkCommandBuffer commands) {
        VkImageMemoryBarrier2 barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER_2;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = target;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.levelCount = 1;
        barrier.subresourceRange.layerCount = 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;

        VkDependencyInfo dependency{};
        dependency.sType = VK_STRUCTURE_TYPE_DEPENDENCY_INFO;
        dependency.imageMemoryBarrierCount = 1;
        dependency.pImageMemoryBarriers = &barrier;
        vkCmdPipelineBarrier2(commands, &dependency);

        VkBufferImageCopy region{};
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.layerCount = 1;
        region.imageExtent = {Grade::SIZE, Grade::SIZE, Grade::SIZE};
        vkCmdCopyBufferToImage(commands, source, target, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcStageMask = VK_PIPELINE_STAGE_2_COPY_BIT;
        barrier.srcAccessMask = VK_ACCESS_2_TRANSFER_WRITE_BIT;
        barrier.dstStageMask = VK_PIPELINE_STAGE_2_FRAGMENT_SHADER_BIT;
        barrier.dstAccessMask = VK_ACCESS_2_SHADER_SAMPLED_READ_BIT;
        vkCmdPipelineBarrier2(commands, &dependency);
    });
}

};  // namespace

/**
 **/
Grade::Grade(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
    VkFormat colour, VkFormat depth, const boost::shared_ptr<image::Image>& strip) :
    context_(context) {
    vulkan::renderer::FullScreen::Spec spec;
    spec.name = "grade";
    spec.fragment.assign(std::begin(gradeShader), std::end(gradeShader));
    spec.sources = 2;
    spec.colour = colour;
    spec.depth = depth;
    pass_ = boost::make_shared<vulkan::renderer::FullScreen>(context_->device(), context_->pipelineCache(),
        context_->resources(), context_->ring(), context_->frameUniforms(), spec);

    std::vector<uint8_t> texels = table(strip);
    if (texels.empty()) {
        if (strip) {
            logger->get()->warn("A grade's strip is {}x{}, not {}x{}, so the grade is the identity", strip->width(),
                strip->height(), SIZE * SIZE, SIZE);
        }
        texels = identity();
    }

    // linear between entries, which is what makes sixteen of them enough
    linear_ = boost::make_shared<vulkan::pipeline::Sampler>(context_->device(), vulkan::pipeline::Sampler::Spec());
    tableTexture_ = createTable(texels);

    // a texel per pixel, so the scene is read exactly rather than filtered
    vulkan::pipeline::Sampler::Spec point;
    point.filter = VK_FILTER_NEAREST;
    point.mipmap = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    nearest_ = boost::make_shared<vulkan::pipeline::Sampler>(context_->device(), point);
}

/**
 **/
Grade::~Grade() = default;

/**
 **/
MaterialHandle Grade::source(const vulkan::frame::RenderTarget& scene, uint32_t slot) {
    vulkan::pipeline::Texture texture = scene.texture(slot);
    if (!texture.image) {
        throw std::runtime_error("A grade's scene has no colour image to read");
    }
    texture.sampler = nearest_;
    const TextureHandle sceneTexture = context_->resources()->add(texture);
    const MaterialHandle material = pass_->source({sceneTexture, tableTexture_});
    sources_[material] = Source{sceneTexture, material};
    return material;
}

/**
 **/
bool Grade::release(const MaterialHandle& source) {
    const std::map<MaterialHandle, Source>::iterator found = sources_.find(source);
    if (found == sources_.end()) {
        return false;
    }
    pass_->release(found->second.current);
    context_->resources()->release(found->second.scene);
    sources_.erase(found);
    return true;
}

/**
 **/
bool Grade::replace(const std::vector<uint8_t>& texels) {
    if (texels.size() != TEXELS * 4) {
        return false;
    }
    const TextureHandle replacement = createTable(texels);
    // every source is paired with the new table before any is let go of its old pairing, so a
    // failure part way leaves the grade on the table it had
    std::vector<MaterialHandle> rebound;
    rebound.reserve(sources_.size());
    try {
        for (const std::pair<const MaterialHandle, Source>& source : sources_) {
            rebound.push_back(pass_->source({source.second.scene, replacement}));
        }
    } catch (...) {
        for (const MaterialHandle& material : rebound) {
            pass_->release(material);
        }
        context_->resources()->release(replacement);
        throw;
    }

    std::size_t next = 0;
    for (std::pair<const MaterialHandle, Source>& source : sources_) {
        pass_->release(source.second.current);
        source.second.current = rebound[next++];
    }
    context_->resources()->release(tableTexture_);
    tableTexture_ = replacement;
    return true;
}

/**
 **/
void Grade::submit(const MaterialHandle& source, Pass* pass) const {
    const std::map<MaterialHandle, Source>::const_iterator found = sources_.find(source);
    pass_->submit(found != sources_.end() ? found->second.current : source, pass);
}

/**
 **/
TextureHandle Grade::createTable(const std::vector<uint8_t>& texels) {
    // UNORM, because the table holds linear colour and is read as it is stored
    vulkan::memory::Image::Spec volume;
    volume.width = SIZE;
    volume.height = SIZE;
    volume.depth = SIZE;
    volume.format = VK_FORMAT_R8G8B8A8_UNORM;
    volume.usage = VK_IMAGE_USAGE_SAMPLED_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT;
    vulkan::pipeline::Texture texture;
    texture.image = boost::make_shared<vulkan::memory::Image>(context_->device(), volume);
    upload(context_, *texture.image, texels);
    texture.sampler = linear_;
    return context_->resources()->add(texture);
}

/**
 **/
std::vector<uint8_t> Grade::table(const boost::shared_ptr<image::Image>& strip) {
    if (!strip || strip->width() != SIZE * SIZE || strip->height() != SIZE || (strip->bpp() != 24 && strip->bpp() != 32)) {
        return std::vector<uint8_t>();
    }

    const std::size_t channels = strip->bpp() / 8;
    const unsigned char* pixels = strip->data();
    std::vector<uint8_t> texels;
    texels.reserve(TEXELS * 4);
    // slice b starts b * SIZE across, and within it red runs across and green down
    for (uint32_t blue = 0; blue < SIZE; blue++) {
        for (uint32_t green = 0; green < SIZE; green++) {
            for (uint32_t red = 0; red < SIZE; red++) {
                const std::size_t at = (static_cast<std::size_t>(green) * strip->width() + static_cast<std::size_t>(blue) * SIZE + red) * channels;
                texels.insert(texels.end(), {pixels[at], pixels[at + 1], pixels[at + 2], 255});
            }
        }
    }
    return texels;
}

/**
 **/
std::vector<uint8_t> Grade::identity() {
    std::vector<uint8_t> texels;
    texels.reserve(TEXELS * 4);
    // each entry is its own position, k of fifteen, which a byte holds exactly as 17k
    for (uint32_t blue = 0; blue < SIZE; blue++) {
        for (uint32_t green = 0; green < SIZE; green++) {
            for (uint32_t red = 0; red < SIZE; red++) {
                texels.insert(texels.end(), {static_cast<uint8_t>(red * 255 / (SIZE - 1)),
                    static_cast<uint8_t>(green * 255 / (SIZE - 1)), static_cast<uint8_t>(blue * 255 / (SIZE - 1)), 255});
            }
        }
    }
    return texels;
}

};  // namespace v3d::render::realtime
