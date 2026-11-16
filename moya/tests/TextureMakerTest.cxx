/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/asset/Writer.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/rib/Sampling.h>
#include <api/rib/Reader.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/output/Chromaticities.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/texture/Level.h>
#include <moya/libmoya/texture/Maker.h>
#include <Imath/ImathVec.h>
#include <Imath/half.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfChromaticities.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfPixelType.h>
#include <OpenEXR/ImfStandardAttributes.h>
#include <OpenEXR/ImfStringAttribute.h>
#include <OpenEXR/ImfTileDescription.h>
#include <OpenEXR/ImfTiledInputFile.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <exception>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Heard.h"

namespace {

namespace film = v3d::rib;
namespace texture = v3d::moya::texture;

/**
 * A directory of its own per case, removed with everything under it, so a case that leaves a file
 * behind fails the case that looks for it rather than the next run.
 **/
class Sandbox {
 public:
    explicit Sandbox(const std::string & name) : path_(boost::filesystem::temp_directory_path() / ("v3d_texture_" + name)) {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
        boost::filesystem::create_directories(path_);
    }

    ~Sandbox() {
        boost::system::error_code ignored;
        boost::filesystem::remove_all(path_, ignored);
    }

    Sandbox(const Sandbox &) = delete;
    Sandbox & operator=(const Sandbox &) = delete;

    std::string file(const std::string & name) const {
        return (path_ / name).string();
    }

 private:
    boost::filesystem::path path_;
};

/** A texture file read back: its header, and each level's values in the order the names give. **/
struct Made {
    Imf::Header header;
    Imf::TileDescription tiles;
    std::vector<texture::Level> levels;
};

/** A level of one channel from its values, row after row. **/
texture::Level levelOf(std::uint32_t width, std::uint32_t height, const std::vector<float> & values) {
    return { .width = width, .height = height, .values = values };
}

/** A making of one filter and its widths, with the same wrap mode along both axes. **/
texture::Making makingOf(texture::Wrap wrap, film::Filter filter, const glm::vec2 & width) {
    texture::Making out;
    out.swrap = wrap;
    out.twrap = wrap;
    out.filter = filter;
    out.width = width;
    return out;
}

/** Writes a scanline EXR file of channels, each texel's values in the order the names give. **/
void writeSource(const std::string & path, std::uint32_t width, std::uint32_t height, const std::vector<std::string> & names,
    const std::vector<float> & values, Imf::PixelType type, const std::optional<Imf::Chromaticities> & chromaticities) {
    Imf::Header header(static_cast<int>(width), static_cast<int>(height));
    for (const std::string & name : names) {
        header.channels().insert(name, Imf::Channel(type));
    }
    if (chromaticities) {
        Imf::addChromaticities(header, *chromaticities);
    }
    // OpenEXR writes a channel only from values of its own type
    std::vector<float> floats = values;
    std::vector<half> halves(values.begin(), values.end());
    const bool whole = type == Imf::FLOAT;
    const std::size_t size = whole ? sizeof(float) : sizeof(half);
    char * const base = whole ? reinterpret_cast<char *>(floats.data()) : reinterpret_cast<char *>(halves.data());
    const std::size_t across = size * names.size();
    Imf::FrameBuffer buffer;
    for (std::size_t i = 0; i < names.size(); ++i) {
        buffer.insert(names[i], Imf::Slice(type, base + (i * size), across, across * width));
    }
    Imf::OutputFile file(path.c_str(), header);
    file.setFrameBuffer(buffer);
    file.writePixels(static_cast<int>(height));
}

/** Writes an 8-bit PNG file of one, three or four channels. **/
void writePng(const std::string & path, std::uint32_t width, std::uint32_t height, std::uint32_t channels, const std::vector<unsigned char> & bytes) {
    const auto image = boost::make_shared<v3d::image::Image>(glm::uvec2(width, height), static_cast<std::uint8_t>(channels * 8U));
    std::copy(bytes.begin(), bytes.end(), image->data());
    v3d::image::Factory factory(boost::make_shared<v3d::log::Logger>());
    const std::optional<std::string> encoded = factory.encode(image, "png");
    BOOST_REQUIRE(encoded.has_value());
    BOOST_REQUIRE(v3d::asset::writeFile(path, *encoded));
}

Made readMade(const std::string & path, const std::vector<std::string> & names) {
    Imf::TiledInputFile file(path.c_str());
    Made out { .header = file.header(), .tiles = file.header().tileDescription(), .levels = {} };
    const std::size_t across = sizeof(float) * names.size();
    for (int level = 0; level < file.numLevels(); ++level) {
        texture::Level read;
        read.width = static_cast<std::uint32_t>(file.levelWidth(level));
        read.height = static_cast<std::uint32_t>(file.levelHeight(level));
        read.values.assign(static_cast<std::size_t>(read.width) * read.height * names.size(), 0.0F);
        Imf::FrameBuffer buffer;
        for (std::size_t i = 0; i < names.size(); ++i) {
            buffer.insert(names[i], Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(read.values.data()) + (i * sizeof(float)), across, across * read.width));
        }
        file.setFrameBuffer(buffer);
        file.readTiles(0, file.numXTiles(level) - 1, 0, file.numYTiles(level) - 1, level);
        out.levels.push_back(read);
    }
    return out;
}

/** The names of a header's channels, in its order. **/
std::vector<std::string> channelsOf(const Imf::Header & header) {
    std::vector<std::string> out;
    for (auto it = header.channels().begin(); it != header.channels().end(); ++it) {
        out.emplace_back(it.name());
    }
    return out;
}

/** The type every channel of a header holds, or nothing when they differ. **/
std::optional<Imf::PixelType> typeOf(const Imf::Header & header) {
    std::optional<Imf::PixelType> out;
    for (auto it = header.channels().begin(); it != header.channels().end(); ++it) {
        if (out && *out != it.channel().type) {
            return std::nullopt;
        }
        out = it.channel().type;
    }
    return out;
}

std::string wrapmodesOf(const Imf::Header & header) {
    const auto * const held = header.findTypedAttribute<Imf::StringAttribute>("wrapmodes");
    return held == nullptr ? std::string() : held->value();
}

/** A log for the maker to write to. **/
boost::shared_ptr<v3d::log::Logger> logOf() {
    return boost::make_shared<v3d::log::Logger>();
}

/** Whether a value lies within a relative tolerance of another, or within it of zero. **/
bool near(float value, float expected, float tolerance) {
    return std::fabs(value - expected) <= tolerance * std::fmax(1.0F, std::fabs(expected));
}


// each texel of a level is the box filter of the two by two texels above it that it covers
BOOST_AUTO_TEST_CASE(moya_texture_each_level_is_the_box_filter_of_the_level_above_test) {
    const std::vector<float> values = { 1.0F, 2.0F, 3.0F, 4.0F, 5.0F, 6.0F, 7.0F, 8.0F };
    const std::vector<texture::Level> levels =
        texture::mipmap(levelOf(4, 2, values), 1, makingOf(texture::Wrap::Black, film::Filter::Box, glm::vec2(1.0F)));
    BOOST_REQUIRE_EQUAL(levels.size(), 3U);
    BOOST_TEST(levels[0].values == values);
    BOOST_TEST(levels[1].width == 2U);
    BOOST_TEST(levels[1].height == 1U);
    BOOST_TEST(levels[1].values[0] == (1.0F + 2.0F + 5.0F + 6.0F) / 4.0F);
    BOOST_TEST(levels[1].values[1] == (3.0F + 4.0F + 7.0F + 8.0F) / 4.0F);
    BOOST_TEST(levels[2].width == 1U);
    BOOST_TEST(levels[2].height == 1U);
    BOOST_TEST(levels[2].values[0] == (levels[1].values[0] + levels[1].values[1]) / 2.0F);

    // each channel is filtered alone
    const std::vector<texture::Level> pairs =
        texture::mipmap(levelOf(2, 1, { 1.0F, 10.0F, 3.0F, 30.0F }), 2, makingOf(texture::Wrap::Black, film::Filter::Box, glm::vec2(1.0F)));
    BOOST_REQUIRE_EQUAL(pairs.size(), 2U);
    BOOST_TEST(pairs[1].values == std::vector<float>({ 2.0F, 20.0F }), boost::test_tools::per_element());
}

// a level is half the one above, rounded down and never below one, as OpenEXR's mipmaps are
BOOST_AUTO_TEST_CASE(moya_texture_levels_halve_rounding_down_test) {
    const std::vector<texture::Level> levels =
        texture::mipmap(levelOf(5, 3, std::vector<float>(15, 0.5F)), 1, makingOf(texture::Wrap::Clamp, film::Filter::Box, glm::vec2(1.0F)));
    BOOST_REQUIRE_EQUAL(levels.size(), 3U);
    BOOST_TEST(levels[1].width == 2U);
    BOOST_TEST(levels[1].height == 1U);
    BOOST_TEST(levels[2].width == 1U);
    BOOST_TEST(levels[2].height == 1U);
    for (const texture::Level & level : levels) {
        for (const float value : level.values) {
            BOOST_TEST(near(value, 0.5F, 1.0e-6F));
        }
    }
}

// a filter that reaches past the edge reads the texels each wrap mode gives there
BOOST_AUTO_TEST_CASE(moya_texture_the_wrap_modes_hold_at_the_edges_test) {
    // a box two texels of the level made wide reads from one texel before the first to the third
    const texture::Level row = levelOf(4, 1, { 1.0F, 2.0F, 3.0F, 4.0F });
    const glm::vec2 wide(2.0F, 1.0F);
    const float periodic = texture::mipmap(row, 1, makingOf(texture::Wrap::Periodic, film::Filter::Box, wide))[1].values[0];
    const float clamp = texture::mipmap(row, 1, makingOf(texture::Wrap::Clamp, film::Filter::Box, wide))[1].values[0];
    const float black = texture::mipmap(row, 1, makingOf(texture::Wrap::Black, film::Filter::Box, wide))[1].values[0];
    BOOST_TEST(periodic == (4.0F + 1.0F + 2.0F + 3.0F) / 4.0F);
    BOOST_TEST(clamp == (1.0F + 1.0F + 2.0F + 3.0F) / 4.0F);
    BOOST_TEST(black == (0.0F + 1.0F + 2.0F + 3.0F) / 4.0F);

    // the t mode wraps the rows, and the s mode does not touch them
    texture::Making down = makingOf(texture::Wrap::Black, film::Filter::Box, glm::vec2(1.0F, 2.0F));
    down.twrap = texture::Wrap::Periodic;
    BOOST_TEST(texture::mipmap(levelOf(1, 4, { 1.0F, 2.0F, 3.0F, 4.0F }), 1, down)[1].values[0] == periodic);

    // an image of one value keeps it at every edge under clamp and periodic, and black darkens it
    const texture::Level even = levelOf(6, 6, std::vector<float>(36, 0.25F));
    for (const texture::Wrap wrap : { texture::Wrap::Clamp, texture::Wrap::Periodic }) {
        for (const texture::Level & level : texture::mipmap(even, 1, makingOf(wrap, film::Filter::Gaussian, glm::vec2(3.0F)))) {
            for (const float value : level.values) {
                BOOST_TEST(near(value, 0.25F, 1.0e-6F), texture::wrapName(wrap));
            }
        }
    }
    const std::vector<texture::Level> darkened = texture::mipmap(even, 1, makingOf(texture::Wrap::Black, film::Filter::Gaussian, glm::vec2(3.0F)));
    BOOST_TEST(darkened[1].values[0] < 0.25F);
    BOOST_TEST(darkened.back().values[0] < 0.25F);
}

// a texture of one texel is its only level, and a filter that reads nothing takes the nearest texel
BOOST_AUTO_TEST_CASE(moya_texture_one_texel_and_an_empty_filter_test) {
    const std::vector<texture::Level> one =
        texture::mipmap(levelOf(1, 1, { 0.75F }), 1, makingOf(texture::Wrap::Black, film::Filter::Gaussian, glm::vec2(4.0F)));
    BOOST_REQUIRE_EQUAL(one.size(), 1U);
    BOOST_TEST(one[0].values[0] == 0.75F);

    // a triangle a tenth of a texel wide reaches no texel centre of the level above
    const std::vector<texture::Level> narrow =
        texture::mipmap(levelOf(4, 1, { 1.0F, 2.0F, 3.0F, 4.0F }), 1, makingOf(texture::Wrap::Black, film::Filter::Triangle, glm::vec2(0.1F)));
    BOOST_TEST(narrow[1].values == std::vector<float>({ 2.0F, 4.0F }), boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(moya_texture_mipmap_refuses_what_it_cannot_filter_test) {
    const texture::Making making;
    BOOST_CHECK_THROW(texture::mipmap(levelOf(1, 1, { 1.0F }), 0, making), std::invalid_argument);
    BOOST_CHECK_THROW(texture::mipmap(levelOf(1, 1, { 1.0F, 1.0F, 1.0F, 1.0F, 1.0F }), 5, making), std::invalid_argument);
    BOOST_CHECK_THROW(texture::mipmap(levelOf(0, 0, {}), 1, making), std::invalid_argument);
    BOOST_CHECK_THROW(texture::mipmap(levelOf(2, 2, { 1.0F }), 1, making), std::invalid_argument);
    BOOST_CHECK_THROW(texture::mipmap(levelOf(1, 1, { 1.0F }), 1, makingOf(texture::Wrap::Black, film::Filter::Box, glm::vec2(0.0F, 1.0F))),
        std::invalid_argument);
    BOOST_CHECK_THROW(
        texture::mipmap(levelOf(1, 1, { 1.0F }), 1, makingOf(texture::Wrap::Black, film::Filter::Box, glm::vec2(1.0F, texture::kWidest * 2.0F))),
        std::invalid_argument);
    BOOST_TEST(texture::widthAllowed(glm::vec2(texture::kWidest)));
}

BOOST_AUTO_TEST_CASE(moya_texture_wrap_names_test) {
    for (const texture::Wrap wrap : { texture::Wrap::Black, texture::Wrap::Clamp, texture::Wrap::Periodic }) {
        texture::Wrap read = texture::Wrap::Black;
        BOOST_TEST(texture::wrapNamed(texture::wrapName(wrap), &read));
        BOOST_TEST((read == wrap));
    }
    texture::Wrap kept = texture::Wrap::Clamp;
    BOOST_TEST(!texture::wrapNamed("mirror", &kept));
    BOOST_TEST((kept == texture::Wrap::Clamp));
}

// a data image keeps its values and its floats, in a tiled file of every level
BOOST_AUTO_TEST_CASE(moya_texture_a_data_texture_keeps_its_values_test) {
    const Sandbox sandbox("data");
    const std::string picture = sandbox.file("height.exr");
    const std::string made = sandbox.file("height.tx");
    const std::vector<float> values = { 0.1F, 0.2F, 0.3F, 0.4F, 0.5F, 0.6F, 0.7F, 0.8F, 0.9F, 1.0F, 1.1F, 1.2F };
    writeSource(picture, 4, 3, { "Y" }, values, Imf::FLOAT, std::nullopt);
    texture::Making making;
    making.swrap = texture::Wrap::Periodic;
    making.twrap = texture::Wrap::Clamp;
    making.colourSpace = "Raw";
    texture::make(picture, made, making, v3d::moya::colour::Config(), logOf());

    BOOST_REQUIRE(boost::filesystem::exists(made));
    BOOST_TEST(!boost::filesystem::exists(made + ".partial"));
    const Made read = readMade(made, { "Y" });
    BOOST_TEST(channelsOf(read.header) == std::vector<std::string>({ "Y" }), boost::test_tools::per_element());
    BOOST_TEST((typeOf(read.header) == Imf::FLOAT));
    BOOST_TEST(read.tiles.xSize == 64U);
    BOOST_TEST(read.tiles.ySize == 64U);
    BOOST_TEST((read.tiles.mode == Imf::MIPMAP_LEVELS));
    BOOST_TEST((read.tiles.roundingMode == Imf::ROUND_DOWN));
    BOOST_TEST(wrapmodesOf(read.header) == "periodic,clamp");
    BOOST_TEST(!Imf::hasChromaticities(read.header));

    const std::vector<texture::Level> expected = texture::mipmap(levelOf(4, 3, values), 1, making);
    BOOST_REQUIRE_EQUAL(read.levels.size(), expected.size());
    for (std::size_t level = 0; level < expected.size(); ++level) {
        BOOST_TEST(read.levels[level].width == expected[level].width);
        BOOST_TEST(read.levels[level].height == expected[level].height);
        BOOST_TEST(read.levels[level].values == expected[level].values, boost::test_tools::per_element());
    }
}

// a colour image is converted into ACEScg before its levels are filtered, and its alpha is kept
BOOST_AUTO_TEST_CASE(moya_texture_a_colour_texture_is_converted_into_acescg_test) {
    const Sandbox sandbox("colour");
    const v3d::moya::colour::Config config;
    const std::string picture = sandbox.file("colour.exr");
    const std::string made = sandbox.file("colour.tx");
    // linear Rec. 709 by its chromaticities, two texels of R, G, B and A
    const auto & xy = v3d::moya::output::kLinearPrimaries[v3d::moya::output::kRec709];
    const Imf::Chromaticities rec709(Imath::V2f(xy[0][0], xy[0][1]), Imath::V2f(xy[1][0], xy[1][1]), Imath::V2f(xy[2][0], xy[2][1]),
        Imath::V2f(xy[3][0], xy[3][1]));
    writeSource(picture, 2, 1, { "R", "G", "B", "A" }, { 1.0F, 0.0F, 0.0F, 0.5F, 0.0F, 0.0F, 1.0F, 1.0F }, Imf::HALF, rec709);
    texture::make(picture, made, texture::Making(), config, logOf());

    const Made read = readMade(made, { "R", "G", "B", "A" });
    BOOST_TEST((typeOf(read.header) == Imf::HALF));
    BOOST_TEST(wrapmodesOf(read.header) == "black,black");
    BOOST_REQUIRE(Imf::hasChromaticities(read.header));
    const Imf::Chromaticities & held = Imf::chromaticities(read.header);
    const auto & ap1 = v3d::moya::output::kLinearPrimaries[v3d::moya::output::kAp1];
    BOOST_TEST(held.red.x == ap1[0][0]);
    BOOST_TEST(held.white.y == ap1[3][1]);

    const v3d::moya::colour::Transform toAcescg = config.input(config.spaceOf(v3d::moya::output::kLinearIds[v3d::moya::output::kRec709]));
    const glm::vec3 red = toAcescg.apply(glm::vec3(1.0F, 0.0F, 0.0F));
    const glm::vec3 blue = toAcescg.apply(glm::vec3(0.0F, 0.0F, 1.0F));
    BOOST_REQUIRE_EQUAL(read.levels.size(), 2U);
    const std::vector<float> & top = read.levels[0].values;
    for (int channel = 0; channel < 3; ++channel) {
        BOOST_TEST(near(top[static_cast<std::size_t>(channel)], red[channel], 2.0e-3F), channel);
        BOOST_TEST(near(top[4U + static_cast<std::size_t>(channel)], blue[channel], 2.0e-3F), channel);
        BOOST_TEST(near(read.levels[1].values[static_cast<std::size_t>(channel)], (red[channel] + blue[channel]) / 2.0F, 2.0e-3F), channel);
    }
    BOOST_TEST(top[3] == 0.5F);
    BOOST_TEST(top[7] == 1.0F);
    BOOST_TEST(read.levels[1].values[3] == 0.75F);
}

// an 8-bit image is read in sRGB, and a grey one is red, green and blue, as data or as colour
BOOST_AUTO_TEST_CASE(moya_texture_an_8_bit_image_is_read_in_srgb_test) {
    const Sandbox sandbox("srgb");
    const v3d::moya::colour::Config config;
    const std::string picture = sandbox.file("grey.png");
    writePng(picture, 2, 2, 1, { 0, 64, 128, 255 });

    const std::string colour = sandbox.file("grey.tx");
    texture::make(picture, colour, texture::Making(), config, logOf());
    const Made read = readMade(colour, { "R", "G", "B" });
    BOOST_TEST(channelsOf(read.header) == std::vector<std::string>({ "B", "G", "R" }), boost::test_tools::per_element());
    BOOST_TEST((typeOf(read.header) == Imf::HALF));
    const std::string srgb = config.spaceOf("srgb_rec709_scene");
    BOOST_REQUIRE(!srgb.empty());
    const glm::vec3 grey = config.input(srgb).apply(glm::vec3(128.0F / 255.0F));
    for (int channel = 0; channel < 3; ++channel) {
        BOOST_TEST(near(read.levels[0].values[6U + static_cast<std::size_t>(channel)], grey[channel], 2.0e-3F), channel);
    }
    BOOST_TEST(near(read.levels[0].values[0], 0.0F, 1.0e-3F));

    texture::Making raw;
    raw.colourSpace = "Raw";
    const std::string data = sandbox.file("grey-data.tx");
    texture::make(picture, data, raw, config, logOf());
    // the PNG reader gives a grey image as red, green and blue
    const Made kept = readMade(data, { "R", "G", "B" });
    BOOST_TEST(channelsOf(kept.header) == std::vector<std::string>({ "B", "G", "R" }), boost::test_tools::per_element());
    BOOST_TEST(!Imf::hasChromaticities(kept.header));
    BOOST_TEST(near(kept.levels[0].values[3], 64.0F / 255.0F, 1.0e-3F));
    BOOST_TEST(near(kept.levels[0].values[7], 128.0F / 255.0F, 1.0e-3F));
}

// an image that cannot be read, or a colour space the configuration lacks, writes no texture
BOOST_AUTO_TEST_CASE(moya_texture_make_refuses_what_it_cannot_read_test) {
    const Sandbox sandbox("refused");
    const v3d::moya::colour::Config config;
    const std::string made = sandbox.file("refused.tx");
    BOOST_CHECK_THROW(texture::make(sandbox.file("absent.png"), made, texture::Making(), config, logOf()), std::exception);

    const std::string picture = sandbox.file("colour.exr");
    writeSource(picture, 1, 1, { "R", "G", "B" }, { 1.0F, 1.0F, 1.0F }, Imf::HALF, std::nullopt);
    texture::Making unknown;
    unknown.colourSpace = "no such space";
    BOOST_CHECK_THROW(texture::make(picture, made, unknown, config, logOf()), std::runtime_error);
    BOOST_TEST(!boost::filesystem::exists(made));
    BOOST_TEST(!boost::filesystem::exists(made + ".partial"));
}

// RIB's MakeTexture makes the texture through the scene client, with its colour space parameter
BOOST_AUTO_TEST_CASE(moya_texture_rib_make_texture_makes_one_test) {
    const Sandbox sandbox("rib");
    const std::string picture = sandbox.file("in.exr");
    const std::string made = sandbox.file("out.tx");
    writeSource(picture, 2, 2, { "Y" }, { 0.0F, 1.0F, 2.0F, 3.0F }, Imf::FLOAT, std::nullopt);
    std::string escaped;
    for (const char letter : picture) {
        escaped += letter == '\\' ? '/' : letter;
    }
    std::string written;
    for (const char letter : made) {
        written += letter == '\\' ? '/' : letter;
    }

    v3d::moya::scene::Scene scene(v3d::moya::scene::kinds());
    const auto logger = boost::make_shared<v3d::log::Logger>();
    const v3d::moya::handling::Heard heard(logger->get());
    v3d::moya::ri::SceneClient client(scene, logger);
    v3d::rib::Reader reader(logger);
    std::istringstream stream("MakeTexture \"" + escaped + "\" \"" + written + "\" \"periodic\" \"mirror\" \"gaussian\" 2 2 \"string colorspace\" \"Raw\"\n");
    BOOST_REQUIRE(reader.read(stream, &client));
    BOOST_TEST(heard.said("wrap mode mirror is not one RI names"));

    const Made read = readMade(written, { "Y" });
    BOOST_TEST(wrapmodesOf(read.header) == "periodic,black");
    texture::Making expected = makingOf(texture::Wrap::Periodic, film::Filter::Gaussian, glm::vec2(2.0F));
    expected.twrap = texture::Wrap::Black;
    BOOST_TEST(read.levels[1].values == texture::mipmap(levelOf(2, 2, { 0.0F, 1.0F, 2.0F, 3.0F }), 1, expected)[1].values,
        boost::test_tools::per_element());

    // a picture that cannot be read is logged, and the file reads on
    std::istringstream absent("MakeTexture \"absent.png\" \"absent.tx\" \"black\" \"black\" \"box\" 1 1\nFrameBegin 1\nFrameEnd\n");
    BOOST_REQUIRE(reader.read(absent, &client));
    BOOST_TEST(heard.said("the texture absent.tx cannot be made from absent.png"));
}

};  // namespace
