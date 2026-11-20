/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/texture/Maker.h>

#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/rib/Sampling.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/film/Filter.h>
#include <moya/libmoya/output/Chromaticities.h>
#include <moya/libmoya/texture/Level.h>
#include <moya/libmoya/texture/internal/Picture.h>
#include <moya/libmoya/texture/internal/Taps.h>
#include <Imath/ImathBox.h>
#include <Imath/ImathVec.h>
#include <Imath/half.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfChromaticities.h>
#include <OpenEXR/ImfCompression.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStandardAttributes.h>
#include <OpenEXR/ImfStringAttribute.h>
#include <OpenEXR/ImfTestFile.h>
#include <OpenEXR/ImfTileDescription.h>
#include <OpenEXR/ImfTiledOutputFile.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/shared_ptr.hpp>

#include <glm/vec2.hpp>


namespace v3d::moya::texture {

namespace {

/** The side of a texture file's tiles, in texels. **/
constexpr int kTile = 64;

/** The Color Interop Forum ID of sRGB, which an image other than an EXR file is read in. **/
constexpr const char * kSrgb = "srgb_rec709_scene";

/** The most an image's values reach for each 8-bit channel. **/
constexpr float kByte = 255.0F;

/** An EXR file's R, G, B and A, or its Y and A. **/
internal::Picture readExrPicture(const std::string & path) {
    Imf::InputFile file(path.c_str());
    const Imf::Header & header = file.header();
    const Imf::ChannelList & list = header.channels();
    std::vector<std::string> names;
    if (list.findChannel("R") != nullptr || list.findChannel("G") != nullptr || list.findChannel("B") != nullptr) {
        names = { "R", "G", "B" };
    } else if (list.findChannel("Y") != nullptr) {
        names = { "Y" };
    } else {
        throw std::runtime_error(path + " holds neither R, G and B nor Y");
    }
    if (list.findChannel("A") != nullptr) {
        names.emplace_back("A");
    }

    internal::Picture out;
    out.exr = true;
    out.channels = static_cast<std::uint32_t>(names.size());
    if (!Imf::hasChromaticities(header)) {
        out.interopId = output::kLinearIds[output::kRec709];
    } else {
        const Imf::Chromaticities & given = Imf::chromaticities(header);
        const float xy[4][2] = {
            { given.red.x, given.red.y }, { given.green.x, given.green.y }, { given.blue.x, given.blue.y }, { given.white.x, given.white.y },
        };
        out.interopId = output::linearIdOf(xy);
    }
    const Imath::Box2i & window = header.dataWindow();
    out.level.width = static_cast<std::uint32_t>(window.max.x - window.min.x + 1);
    out.level.height = static_cast<std::uint32_t>(window.max.y - window.min.y + 1);
    out.level.values.assign(static_cast<std::size_t>(out.level.width) * out.level.height * out.channels, 0.0F);

    // a slice is addressed by the data window's coordinates, so its base lies before the values
    const std::size_t across = sizeof(float) * out.channels;
    const std::ptrdiff_t origin = -static_cast<std::ptrdiff_t>(window.min.x) - (static_cast<std::ptrdiff_t>(window.min.y) * out.level.width);
    char * const base = reinterpret_cast<char *>(out.level.values.data()) + (origin * static_cast<std::ptrdiff_t>(across));
    Imf::FrameBuffer buffer;
    for (std::size_t i = 0; i < names.size(); ++i) {
        // a channel the file lacks is filled with zero
        const Imf::Channel * const held = list.findChannel(names[i]);
        out.floats = out.floats || (held != nullptr && held->type != Imf::HALF);
        buffer.insert(names[i], Imf::Slice(Imf::FLOAT, base + (i * sizeof(float)), across, across * out.level.width, 1, 1, 0.0));
    }
    file.setFrameBuffer(buffer);
    file.readPixels(window.min.y, window.max.y);
    return out;
}

/** A PNG, JPEG, TGA or BMP file's channels, each of 8 bits, as values from 0 to 1. **/
internal::Picture readOtherPicture(const std::string & path, const boost::shared_ptr<log::Logger> & logger) {
    image::Factory factory(logger);
    const boost::shared_ptr<image::Image> read = factory.read(path);
    if (!read || read->data() == nullptr) {
        throw std::runtime_error(path + " cannot be read as an image");
    }
    internal::Picture out;
    out.channels = static_cast<std::uint32_t>(read->format());
    if (read->bpp() != out.channels * 8U) {
        throw std::runtime_error(path + " does not hold 8 bits in each channel");
    }
    out.level.width = read->width();
    out.level.height = read->height();
    const std::size_t count = static_cast<std::size_t>(out.level.width) * out.level.height * out.channels;
    const unsigned char * const bytes = read->data();
    out.level.values.resize(count);
    for (std::size_t i = 0; i < count; ++i) {
        out.level.values[i] = static_cast<float>(bytes[i]) / kByte;
    }
    return out;
}

/**
 * The colour space a picture is read in, or empty when its values are data.
 **/
std::string spaceOf(const internal::Picture & picture, const std::string & path, const Making & making, const colour::Config & config,
    const boost::shared_ptr<log::Logger> & logger) {
    if (making.data) {
        return {};
    }
    if (!making.colourSpace.empty()) {
        return config.data(making.colourSpace) ? std::string() : making.colourSpace;
    }
    std::string id = picture.exr ? picture.interopId : std::string(kSrgb);
    if (id.empty()) {
        logger->get()->warn("the image {} has chromaticities of no space moya knows, so it is read as linear Rec. 709", path);
        id = output::kLinearIds[output::kRec709];
    }
    const std::string space = config.spaceOf(id);
    if (space.empty()) {
        throw std::runtime_error("the colour configuration has no colour space of ID " + id + ", which " + path + " is in");
    }
    return space;
}

/** A grey picture with its grey made red, green and blue, and its alpha kept. **/
void spread(internal::Picture * picture) {
    const std::uint32_t from = picture->channels;
    const std::uint32_t to = from + 2U;
    const std::size_t texels = static_cast<std::size_t>(picture->level.width) * picture->level.height;
    std::vector<float> values(texels * to);
    for (std::size_t texel = 0; texel < texels; ++texel) {
        const float * const read = &picture->level.values[texel * from];
        float * const written = &values[texel * to];
        written[0] = read[0];
        written[1] = read[0];
        written[2] = read[0];
        if (from == 2U) {
            written[3] = read[1];
        }
    }
    picture->level.values = std::move(values);
    picture->channels = to;
}

/**
 * Each texel's taps along one axis.
 *
 * @param above the axis's texels in the level above
 * @param made the axis's texels in the level being made
 * @param width the filter's whole extent, in texels of the level being made
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the level above, then the level made
std::vector<internal::Taps> tapsOf(std::uint32_t above, std::uint32_t made, float width) {
    const double ratio = static_cast<double>(above) / made;
    const double reach = 0.5 * width * ratio;
    std::vector<internal::Taps> out(made);
    for (std::uint32_t i = 0; i < made; ++i) {
        const double centre = (i + 0.5) * ratio;
        internal::Taps & taps = out[i];
        // a texel is read when its centre, k + 0.5, lies within the filter's reach
        taps.first = static_cast<std::int64_t>(std::ceil(centre - reach - 0.5));  // checked: bounded by the level and kWidest
        const auto last = static_cast<std::int64_t>(std::floor(centre + reach - 0.5));  // checked: bounded by the level and kWidest
        for (std::int64_t k = taps.first; k <= last; ++k) {
            taps.offsets.push_back(static_cast<float>((static_cast<double>(k) + 0.5 - centre) / ratio));
        }
        taps.nearest = std::min(static_cast<std::int64_t>(above) - 1, static_cast<std::int64_t>(std::floor(centre)));  // checked: within the level
    }
    return out;
}

/** The texel an index along an axis of a size reads by a wrap mode, or -1 for a black one. **/
std::int64_t wrapped(std::int64_t index, std::int64_t size, Wrap wrap) {
    if (index >= 0 && index < size) {
        return index;
    }
    switch (wrap) {
    case Wrap::Periodic:
        return ((index % size) + size) % size;
    case Wrap::Clamp:
        return index < 0 ? 0 : size - 1;
    case Wrap::Black:
        break;
    }
    return -1;
}

/**
 * One texel of the level being made, from the texels of the level above its taps read, into
 * written. sums holds a double for each channel.
 **/
void filterTexel(const Level & above, std::uint32_t channels, const Making & making, const internal::Taps & down, const internal::Taps & across,
    std::vector<double> & sums, float * written) {
    std::fill(sums.begin(), sums.end(), 0.0);
    double total = 0.0;
    for (std::size_t y = 0; y < down.offsets.size(); ++y) {
        const std::int64_t readRow = wrapped(down.first + static_cast<std::int64_t>(y), above.height, making.twrap);
        for (std::size_t x = 0; x < across.offsets.size(); ++x) {
            const float weight = film::Filter::weight(making.filter, glm::vec2(across.offsets[x], down.offsets[y]), making.width);
            total += weight;
            const std::int64_t readColumn = wrapped(across.first + static_cast<std::int64_t>(x), above.width, making.swrap);
            if (readRow < 0 || readColumn < 0) {
                continue;
            }
            const float * const read = &above.values[((static_cast<std::size_t>(readRow) * above.width) + static_cast<std::size_t>(readColumn)) * channels];
            for (std::uint32_t channel = 0; channel < channels; ++channel) {
                sums[channel] += static_cast<double>(weight) * read[channel];
            }
        }
    }
    if (total == 0.0) {
        const float * const read = &above.values[((static_cast<std::size_t>(down.nearest) * above.width) + static_cast<std::size_t>(across.nearest)) * channels];
        std::copy(read, read + channels, written);
        return;
    }
    for (std::uint32_t channel = 0; channel < channels; ++channel) {
        written[channel] = static_cast<float>(sums[channel] / total);
    }
}

/** The level below one, half its size along each side, rounded down and never below one. **/
Level reduced(const Level & above, std::uint32_t channels, const Making & making) {
    Level out;
    out.width = std::max(1U, above.width / 2U);
    out.height = std::max(1U, above.height / 2U);
    out.values.resize(static_cast<std::size_t>(out.width) * out.height * channels);
    const std::vector<internal::Taps> columns = tapsOf(above.width, out.width, making.width.x);
    const std::vector<internal::Taps> rows = tapsOf(above.height, out.height, making.width.y);
    tbb::parallel_for(tbb::blocked_range<std::uint32_t>(0, out.height), [&](const tbb::blocked_range<std::uint32_t> & range) {
        std::vector<double> sums(channels);
        for (std::uint32_t row = range.begin(); row != range.end(); ++row) {
            for (std::uint32_t column = 0; column < out.width; ++column) {
                filterTexel(above, channels, making, rows[row], columns[column], sums,
                    &out.values[((static_cast<std::size_t>(row) * out.width) + column) * channels]);
            }
        }
    });
    return out;
}

/** The names of a texture's channels: R, G, B and A, or a data texture's Y, or Y and A. **/
std::vector<std::string> channelNames(std::uint32_t channels) {
    switch (channels) {
    case 1U:
        return { "Y" };
    case 2U:
        return { "Y", "A" };
    case 3U:
        return { "R", "G", "B" };
    default:
        return { "R", "G", "B", "A" };
    }
}

/** Writes a texture's levels to a tiled EXR file. **/
void write(const std::string & path, const std::vector<Level> & levels, std::uint32_t channels, bool floats, bool colour, const Making & making) {
    const Level & top = levels.front();
    Imf::Header header(static_cast<int>(top.width), static_cast<int>(top.height));
    header.compression() = Imf::ZIP_COMPRESSION;
    header.setTileDescription(Imf::TileDescription(kTile, kTile, Imf::MIPMAP_LEVELS, Imf::ROUND_DOWN));
    const std::vector<std::string> names = channelNames(channels);
    for (const std::string & name : names) {
        header.channels().insert(name, Imf::Channel(floats ? Imf::FLOAT : Imf::HALF));
    }
    header.insert("wrapmodes", Imf::StringAttribute(wrapName(making.swrap) + "," + wrapName(making.twrap)));
    if (colour) {
        const auto & xy = output::kLinearPrimaries[output::kAp1];
        Imf::addChromaticities(header, Imf::Chromaticities(Imath::V2f(xy[0][0], xy[0][1]), Imath::V2f(xy[1][0], xy[1][1]),
            Imath::V2f(xy[2][0], xy[2][1]), Imath::V2f(xy[3][0], xy[3][1])));
    }

    Imf::TiledOutputFile file(path.c_str(), header);
    if (static_cast<std::size_t>(file.numLevels()) != levels.size()) {
        throw std::logic_error("a texture's levels are not the levels OpenEXR holds");
    }
    // OpenEXR writes a channel only from values of its own type, so halves are converted first
    const Imf::PixelType type = floats ? Imf::FLOAT : Imf::HALF;
    const std::size_t size = floats ? sizeof(float) : sizeof(half);
    const std::size_t across = size * channels;
    std::vector<half> halves;
    for (int level = 0; level < file.numLevels(); ++level) {
        const Level & held = levels[static_cast<std::size_t>(level)];
        if (!floats) {
            halves.assign(held.values.begin(), held.values.end());
        }
        // OpenEXR reads through a pointer that is not const, and writing a file reads nothing back
        char * const base = floats ? const_cast<char *>(reinterpret_cast<const char *>(held.values.data())) : reinterpret_cast<char *>(halves.data());
        Imf::FrameBuffer buffer;
        for (std::size_t i = 0; i < names.size(); ++i) {
            buffer.insert(names[i], Imf::Slice(type, base + (i * size), across, across * held.width));
        }
        file.setFrameBuffer(buffer);
        file.writeTiles(0, file.numXTiles(level) - 1, 0, file.numYTiles(level) - 1, level);
    }
}

};  // namespace

bool wrapNamed(const std::string & name, Wrap * wrap) {
    constexpr Wrap every[] = { Wrap::Black, Wrap::Clamp, Wrap::Periodic };
    const auto * const found = std::ranges::find_if(every, [&name](Wrap each) { return name == wrapName(each); });
    if (found == std::ranges::end(every)) {
        return false;
    }
    *wrap = *found;
    return true;
}

std::string wrapName(Wrap wrap) {
    switch (wrap) {
    case Wrap::Black:
        return "black";
    case Wrap::Clamp:
        return "clamp";
    case Wrap::Periodic:
        return "periodic";
    }
    return {};
}

bool widthAllowed(const glm::vec2 & width) {
    return rib::filterWidth(width) && width.x <= kWidest && width.y <= kWidest;
}

std::vector<Level> mipmap(const Level & image, std::uint32_t channels, const Making & making) {
    if (channels < 1U || channels > 4U) {
        throw std::invalid_argument("a texture holds from 1 to 4 channels");
    }
    if (image.width == 0U || image.height == 0U || image.values.size() != static_cast<std::size_t>(image.width) * image.height * channels) {
        throw std::invalid_argument("an image with no texels, or with values that do not fill it");
    }
    if (!widthAllowed(making.width)) {
        throw std::invalid_argument("a texture's filter widths have to be above zero and at most 64");
    }
    std::vector<Level> out { image };
    while (out.back().width > 1U || out.back().height > 1U) {
        Level next = reduced(out.back(), channels, making);
        out.push_back(std::move(next));
    }
    return out;
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the image read, then the texture written
void make(const std::string & picture, const std::string & texture, const Making & making, const colour::Config & config,
    const boost::shared_ptr<log::Logger> & logger) {
    if (!widthAllowed(making.width)) {
        throw std::invalid_argument("a texture's filter widths have to be above zero and at most 64");
    }
    internal::Picture read = Imf::isOpenExrFile(picture.c_str()) ? readExrPicture(picture) : readOtherPicture(picture, logger);
    const std::string space = spaceOf(read, picture, making, config, logger);
    const bool colour = !space.empty();
    if (colour && !read.level.values.empty()) {
        if (read.channels < 3U) {
            spread(&read);
        }
        config.input(space).apply(read.level.values.data(), read.level.values.size() / read.channels, read.channels);
    }
    const std::vector<Level> levels = mipmap(read.level, read.channels, making);

    const std::string partial = texture + ".partial";
    try {
        write(partial, levels, read.channels, read.floats, colour, making);
        boost::filesystem::rename(partial, texture);
    } catch (...) {
        boost::system::error_code ignored;
        boost::filesystem::remove(partial, ignored);
        throw;
    }
}

};  // namespace v3d::moya::texture
