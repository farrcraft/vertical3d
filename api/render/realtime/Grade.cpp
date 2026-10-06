/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Grade.h"

#include <api/render/realtime/DeviceContext.h>
#include <api/render/realtime/vulkan/frame/RenderTarget.h>
#include <api/render/realtime/vulkan/memory/TextureFactory.h>
#include <api/render/realtime/vulkan/pipeline/Sampler.h>
#include <api/render/realtime/vulkan/renderer/FullScreen.h>

#include <cstddef>
#include <exception>
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

};  // namespace

/**
 **/
Grade::Grade(const boost::shared_ptr<log::Logger>& logger, const boost::shared_ptr<DeviceContext>& context,
    VkFormat colour, VkFormat depth, const boost::shared_ptr<image::Image>& strip) :
    logger_(logger),
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

    tableTexture_ = createTable(texels);

    // a texel per pixel, so the scene is read exactly rather than filtered
    vulkan::pipeline::Sampler::Spec point;
    point.filter = VK_FILTER_NEAREST;
    point.mipmap = VK_SAMPLER_MIPMAP_MODE_NEAREST;
    nearest_ = boost::make_shared<vulkan::pipeline::Sampler>(context_->device(), point);
}

/**
 **/
Grade::~Grade() {
    // a destructor must not throw. Releasing only hands objects to the ring, which fails only
    // when out of memory, and then they stay registered until the context goes
    try {
        for (const std::pair<const MaterialHandle, Source>& source : sources_) {
            pass_->release(source.second.current);
            context_->resources()->release(source.second.scene);
        }
        sources_.clear();
        context_->resources()->release(tableTexture_);
    } catch (const std::exception& error) {
        logger_->get()->error("A grade could not release its table and sources: {}", error.what());
    }
}

/**
 **/
MaterialHandle Grade::source(const vulkan::frame::RenderTarget& scene, uint32_t slot) {
    vulkan::pipeline::Texture texture = scene.texture(slot);
    if (!texture.image) {
        throw std::runtime_error("A grade's scene has no colour image to read");
    }
    texture.sampler = nearest_;
    const TextureHandle sceneTexture = context_->resources()->add(texture);
    MaterialHandle material;
    try {
        material = pass_->source({sceneTexture, tableTexture_});
        sources_[material] = Source{sceneTexture, material};
    } catch (...) {
        if (material.valid()) {
            pass_->release(material);
        }
        context_->resources()->release(sceneTexture);
        throw;
    }
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
    // every source is paired with the new table before any old pairing is released, so a
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
    // sampled linearly between entries, the factory's default, so sixteen of them are enough
    const vulkan::memory::TextureFactory factory(context_->device(), context_->uploader());
    return context_->resources()->add(factory.volume(texels.data(), SIZE, SIZE, SIZE));
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
