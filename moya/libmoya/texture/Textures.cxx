/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/texture/Textures.h>

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/cpu/Textures.h>
#include <moya/libmoya/device/Device.h>
#include <moya/libmoya/kernel/texture/Catalog.h>
#include <moya/libmoya/kernel/texture/Image.h>
#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/output/Chromaticities.h>
#include <moya/libmoya/texture/Cache.h>
#include <moya/libmoya/texture/Maker.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Request.h>
#include <moya/libmoya/texture/SearchPath.h>
#include <OpenEXR/ImfChromaticities.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfStandardAttributes.h>
#include <OpenEXR/ImfTestFile.h>
#include <OpenEXR/ImfTileDescription.h>
#include <OpenEXR/ImfTiledInputFile.h>

#include <cstddef>
#include <cstdint>
#include <exception>
#include <fstream>
#include <ios>
#include <iterator>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::moya::texture {

namespace {

/** The token a UDIM image's path holds in place of its tile's number. **/
constexpr const char * kUdim = "<UDIM>";

/** A UDIM grid's columns, its most rows, and the number of its first tile. **/
constexpr std::uint32_t kUdimColumns = 10;
constexpr std::uint32_t kUdimRows = 10;
constexpr std::uint32_t kFirstUdim = 1001;

/** FNV-1a's 64-bit offset basis and prime. **/
constexpr std::uint64_t kFnvOffset = 0xCBF29CE484222325ULL;
constexpr std::uint64_t kFnvPrime = 0x100000001B3ULL;

/** How a request is read, as the key a texture made from it is named by. **/
std::string readingOf(const Request & request) {
    return request.colour ? "colour " + request.colourSpace : std::string("data");
}

/** A hash of a file's bytes and of how it is read. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the file, then how it is read
std::uint64_t hashOf(const std::string & path, const std::string & reading) {
    std::ifstream stream(path, std::ios::binary);
    std::uint64_t hash = kFnvOffset;
    const auto add = [&hash](char letter) {
        hash ^= static_cast<unsigned char>(letter);
        hash *= kFnvPrime;
    };
    for (std::istreambuf_iterator<char> it(stream), end; it != end; ++it) {
        add(*it);
    }
    add('\n');
    for (const char letter : reading) {
        add(letter);
    }
    return hash;
}

/** A hash as sixteen hexadecimal digits. **/
std::string hexOf(std::uint64_t value) {
    constexpr const char * kDigits = "0123456789abcdef";
    constexpr std::size_t kNibbles = 16;
    constexpr std::uint64_t kNibble = 0xFU;
    constexpr std::uint64_t kNibbleBits = 4U;
    std::string out(kNibbles, '0');
    for (std::size_t i = kNibbles; i > 0; --i) {
        out[i - 1U] = kDigits[value & kNibble];
        value >>= kNibbleBits;
    }
    return out;
}

/**
 * Whether a file is a texture read as it is: a tiled EXR file of every level rounded down, read
 * as data or with ACEScg's chromaticities.
 **/
bool ready(const std::string & path, const Request & request) {
    bool tiled = false;
    if (!Imf::isOpenExrFile(path.c_str(), tiled) || !tiled) {
        return false;
    }
    const Imf::TiledInputFile file(path.c_str());
    const Imf::Header & header = file.header();
    const Imf::TileDescription & tiles = header.tileDescription();
    if (tiles.mode != Imf::MIPMAP_LEVELS || tiles.roundingMode != Imf::ROUND_DOWN) {
        return false;
    }
    if (!request.colour) {
        return true;
    }
    if (!Imf::hasChromaticities(header)) {
        return false;
    }
    const Imf::Chromaticities & given = Imf::chromaticities(header);
    const float xy[4][2] = {
        { given.red.x, given.red.y }, { given.green.x, given.green.y }, { given.blue.x, given.blue.y }, { given.white.x, given.white.y },
    };
    return output::linearIdOf(xy) == output::kLinearIds[output::kAp1];
}

template <typename T>
boost::shared_ptr<device::Buffer> uploaded(device::Device & device, const std::vector<T> & values) {
    const std::size_t bytes = values.size() * sizeof(T);
    auto buffer = device.allocate(bytes);
    device.upload(*buffer, values.data(), bytes);
    return buffer;
}

};  // namespace

Textures::Textures(device::Device & device, const std::vector<Request> & images, const Options & options, const colour::Config & config,
    const boost::shared_ptr<log::Logger> & logger) :
    options_(options), config_(config), logger_(logger), cache_(boost::make_shared<Cache>(options.memory)) {
    for (const Request & request : images) {
        kernel::texture::Image grid;
        grid.first = static_cast<std::uint32_t>(tiles_.size());
        const std::size_t token = request.file.find(kUdim);
        if (token == std::string::npos) {
            tiles_.push_back(textureOf(request.file, request));
            images_.push_back(grid);
            continue;
        }
        std::vector<std::uint32_t> udims(static_cast<std::size_t>(kUdimColumns) * kUdimRows, kernel::texture::kNoTexture);
        std::uint32_t rows = 0;
        for (std::uint32_t row = 0; row < kUdimRows; ++row) {
            for (std::uint32_t column = 0; column < kUdimColumns; ++column) {
                std::string file = request.file;
                file.replace(token, std::string(kUdim).size(), std::to_string(kFirstUdim + column + (row * kUdimColumns)));
                if (!found(file, options_.searchPath)) {
                    continue;
                }
                udims[(static_cast<std::size_t>(row) * kUdimColumns) + column] = textureOf(file, request);
                rows = row + 1U;
            }
        }
        if (rows == 0U) {
            logger_->get()->warn("the UDIM image {} has no tile, so its node gives its default", request.file);
            rows = 1U;
        }
        grid.columns = kUdimColumns;
        grid.rows = rows;
        tiles_.insert(tiles_.end(), udims.begin(), udims.begin() + (static_cast<std::ptrdiff_t>(rows) * kUdimColumns));
        images_.push_back(grid);
    }
    imageBuffer_ = uploaded(device, images_);
    tileBuffer_ = uploaded(device, tiles_);
    textureBuffer_ = uploaded(device, cache_->textures());
}

device::cpu::Textures Textures::view() const {
    return device::cpu::Textures { .catalog = kernel::texture::Catalog { .images = device::view<kernel::texture::Image>(*imageBuffer_),
                                       .tiles = device::view<std::uint32_t>(*tileBuffer_),
                                       .textures = device::view<kernel::texture::Texture>(*textureBuffer_) },
        .cache = cache_.get(),
        .fetch = &Cache::fetch };
}

void Textures::report() const {
    for (const std::string & failure : cache_->failures()) {
        logger_->get()->error("a texel could not be read, and was taken as zero: {}", failure);
    }
}

std::uint32_t Textures::textureOf(const std::string & file, const Request & request) {
    const std::optional<std::string> path = found(file, options_.searchPath);
    if (!path) {
        logger_->get()->warn("the image {} is not found, so its node gives its default", file);
        return kernel::texture::kNoTexture;
    }
    const std::pair<std::string, std::string> key { *path, readingOf(request) };
    const auto held = opened_.find(key);
    if (held != opened_.end()) {
        return held->second;
    }
    std::uint32_t out = kernel::texture::kNoTexture;
    try {
        out = cache_->open(ready(*path, request) ? *path : madeFrom(*path, request));
    } catch (const std::exception & error) {
        logger_->get()->warn("the image {} cannot be read as a texture, so its node gives its default: {}", *path, error.what());
    }
    opened_.emplace(key, out);
    return out;
}

std::string Textures::madeFrom(const std::string & path, const Request & request) {
    const boost::filesystem::path directory =
        options_.directory.empty() ? boost::filesystem::temp_directory_path() / "moya-textures" : boost::filesystem::path(options_.directory);
    boost::filesystem::create_directories(directory);
    const std::string made = (directory / (hexOf(hashOf(path, readingOf(request))) + ".exr")).string();
    boost::system::error_code ignored;
    if (!boost::filesystem::exists(made, ignored)) {
        Making making;
        making.colourSpace = request.colourSpace;
        making.data = !request.colour;
        make(path, made, making, config_, logger_);
        logger_->get()->debug("made the texture {} from {}", made, path);
    }
    return made;
}

};  // namespace v3d::moya::texture
