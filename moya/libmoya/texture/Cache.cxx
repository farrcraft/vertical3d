/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/texture/Cache.h>

#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/texture/internal/Opened.h>
#include <moya/libmoya/texture/internal/Tile.h>
#include <moya/libmoya/texture/internal/TileKey.h>
#include <Imath/ImathBox.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfTileDescription.h>
#include <OpenEXR/ImfTiledInputFile.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/vec4.hpp>

namespace v3d::moya::texture {

namespace {

/** The channels a texel is held as. **/
constexpr std::size_t kHeld = 4;

/** The channels of a header a texture reads: R, G, B and A, or Y and A, each where it has them. **/
std::vector<std::string> channelsOf(const Imf::Header & header, const std::string & path) {
    const Imf::ChannelList & list = header.channels();
    std::vector<std::string> out;
    if (list.findChannel("R") != nullptr || list.findChannel("G") != nullptr || list.findChannel("B") != nullptr) {
        out = { "R", "G", "B" };
    } else if (list.findChannel("Y") != nullptr) {
        out = { "Y" };
    } else {
        throw std::runtime_error(path + " holds neither R, G and B nor Y");
    }
    if (list.findChannel("A") != nullptr) {
        out.emplace_back("A");
    }
    return out;
}

};  // namespace

Cache::Cache(std::size_t bytes) : limit_(bytes) {
}

std::uint32_t Cache::open(const std::string & path) {
    auto opened = boost::make_shared<internal::Opened>();
    opened->path = path;
    opened->file = boost::make_shared<Imf::TiledInputFile>(path.c_str());
    const Imf::Header & header = opened->file->header();
    const Imf::TileDescription & tiles = header.tileDescription();
    if (tiles.mode != Imf::MIPMAP_LEVELS || tiles.roundingMode != Imf::ROUND_DOWN) {
        throw std::runtime_error(path + " is not a texture of every level rounded down");
    }
    opened->channels = channelsOf(header, path);
    opened->tileWidth = tiles.xSize;
    opened->tileHeight = tiles.ySize;
    const Imath::Box2i & window = header.dataWindow();
    const kernel::texture::Texture texture { .width = static_cast<std::uint32_t>(window.max.x - window.min.x + 1),
        .height = static_cast<std::uint32_t>(window.max.y - window.min.y + 1),
        .levels = static_cast<std::uint32_t>(opened->file->numLevels()) };
    const std::lock_guard<std::mutex> lock(mutex_);
    files_.push_back(opened);
    textures_.push_back(texture);
    return static_cast<std::uint32_t>(textures_.size() - 1U);
}

void Cache::fetch(void * cache, std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row, float * out) noexcept {
    auto * const held = static_cast<Cache *>(cache);
    try {
        held->read(texture, level, column, row, out);
    } catch (const std::exception & error) {
        std::fill(out, out + kHeld, 0.0F);
        try {
            const std::lock_guard<std::mutex> lock(held->mutex_);
            held->failures_.insert(held->files_.at(texture)->path + ": " + error.what());
        } catch (...) {  // NOLINT(bugprone-empty-catch) - a failure that cannot be recorded leaves the texel zero
        }
    }
}

std::vector<std::string> Cache::failures() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return { failures_.begin(), failures_.end() };
}

std::uint64_t Cache::loads() const {
    const std::lock_guard<std::mutex> lock(mutex_);
    return loads_;
}

void Cache::read(std::uint32_t texture, std::uint32_t level, std::uint32_t column, std::uint32_t row, float * out) {
    const internal::Opened & opened = *files_.at(texture);
    const internal::TileKey key { .texture = texture, .level = level, .column = column / opened.tileWidth, .row = row / opened.tileHeight };
    const std::uint32_t across = column % opened.tileWidth;
    const std::uint32_t down = row % opened.tileHeight;
    {
        const std::lock_guard<std::mutex> lock(mutex_);
        const auto found = tiles_.find(key);
        if (found != tiles_.end()) {
            copyOut(found->second, across, down, out);
            return;
        }
    }
    internal::Tile loaded = load(key);
    const std::lock_guard<std::mutex> lock(mutex_);
    ++loads_;
    auto found = tiles_.find(key);
    if (found == tiles_.end()) {
        // another thread may have loaded the same tile meanwhile, and its copy is kept
        held_ += loaded.texels.size() * sizeof(float);
        used_.push_front(key);
        loaded.used = used_.begin();
        found = tiles_.emplace(key, std::move(loaded)).first;
        while (held_ > limit_ && used_.size() > 1U) {
            const auto dropped = tiles_.find(used_.back());
            held_ -= dropped->second.texels.size() * sizeof(float);
            tiles_.erase(dropped);
            used_.pop_back();
        }
    }
    copyOut(found->second, across, down, out);
}

void Cache::copyOut(internal::Tile & tile, std::uint32_t column, std::uint32_t row, float * out) {
    used_.splice(used_.begin(), used_, tile.used);
    const float * const texel = &tile.texels[((static_cast<std::size_t>(row) * tile.width) + column) * kHeld];
    std::copy(texel, texel + kHeld, out);
}

internal::Tile Cache::load(const internal::TileKey & key) {
    internal::Opened & opened = *files_.at(key.texture);
    const std::lock_guard<std::mutex> lock(opened.reading);
    Imf::TiledInputFile & file = *opened.file;
    const Imath::Box2i box = file.dataWindowForTile(static_cast<int>(key.column), static_cast<int>(key.row), static_cast<int>(key.level));
    const auto width = static_cast<std::uint32_t>(box.max.x - box.min.x + 1);
    const auto height = static_cast<std::uint32_t>(box.max.y - box.min.y + 1);
    const auto channels = static_cast<std::uint32_t>(opened.channels.size());
    std::vector<float> read(static_cast<std::size_t>(width) * height * channels, 0.0F);
    // a slice is addressed by the level's coordinates, so its base lies before the tile's values
    const std::size_t stride = sizeof(float) * channels;
    const std::ptrdiff_t origin = -static_cast<std::ptrdiff_t>(box.min.x) - (static_cast<std::ptrdiff_t>(box.min.y) * width);
    char * const base = reinterpret_cast<char *>(read.data()) + (origin * static_cast<std::ptrdiff_t>(stride));
    Imf::FrameBuffer buffer;
    for (std::size_t i = 0; i < opened.channels.size(); ++i) {
        buffer.insert(opened.channels[i], Imf::Slice(Imf::FLOAT, base + (i * sizeof(float)), stride, stride * width, 1, 1, 0.0));
    }
    file.setFrameBuffer(buffer);
    file.readTile(static_cast<int>(key.column), static_cast<int>(key.row), static_cast<int>(key.level));

    internal::Tile out;
    out.width = width;
    out.texels.resize(static_cast<std::size_t>(width) * height * kHeld);
    for (std::size_t texel = 0; texel < static_cast<std::size_t>(width) * height; ++texel) {
        const glm::vec4 value = kernel::texture::expanded(&read[texel * channels], channels);
        for (std::size_t channel = 0; channel < kHeld; ++channel) {
            out.texels[(texel * kHeld) + channel] = value[static_cast<int>(channel)];
        }
    }
    return out;
}

};  // namespace v3d::moya::texture
