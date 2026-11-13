/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Textures.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/surface/TextureFootprint.h>
#include <moya/libmoya/kernel/texture/Address.h>
#include <moya/libmoya/kernel/texture/Catalog.h>
#include <moya/libmoya/kernel/texture/Filtering.h>
#include <moya/libmoya/kernel/texture/Image.h>
#include <moya/libmoya/kernel/texture/Lookup.h>
#include <moya/libmoya/kernel/texture/Resident.h>
#include <moya/libmoya/kernel/texture/Sampling.h>
#include <moya/libmoya/kernel/texture/Texels.h>
#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/texture/Level.h>
#include <moya/libmoya/texture/Maker.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Request.h>
#include <moya/libmoya/texture/Textures.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfPixelType.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace {

namespace kt = v3d::moya::kernel::texture;
namespace texture = v3d::moya::texture;

/**
 * Textures held whole, each the levels mipmap() makes of an image of one channel, behind a
 * kernel::texture::Resident that reads them. Each image is a grid of one texture, in order.
 **/
class Held {
 public:
    /** Adds a texture of one channel, filtered by a box, as an image of its own. **/
    std::uint32_t add(std::uint32_t width, std::uint32_t height, const std::vector<float> & values) {
        const std::vector<texture::Level> levels = texture::mipmap(texture::Level { .width = width, .height = height, .values = values }, 1, texture::Making());
        starts_.push_back(static_cast<std::uint32_t>(texels_.size()));
        for (const texture::Level & level : levels) {
            for (const float value : level.values) {
                texels_.push_back(kt::expanded(&value, 1U));
            }
        }
        textures_.push_back(kt::Texture { .width = width, .height = height, .levels = static_cast<std::uint32_t>(levels.size()) });
        const auto at = static_cast<std::uint32_t>(textures_.size() - 1U);
        images_.push_back(kt::Image { .first = static_cast<std::uint32_t>(tiles_.size()) });
        tiles_.push_back(at);
        return static_cast<std::uint32_t>(images_.size() - 1U);
    }

    /** Adds a UDIM image of textures already added, ten columns to a row, kNoTexture for a gap. **/
    std::uint32_t grid(const std::vector<std::uint32_t> & textures, std::uint32_t rows) {
        images_.push_back(kt::Image { .first = static_cast<std::uint32_t>(tiles_.size()), .columns = 10, .rows = rows });
        tiles_.insert(tiles_.end(), textures.begin(), textures.end());
        return static_cast<std::uint32_t>(images_.size() - 1U);
    }

    /** The texture an image added alone reads. **/
    std::uint32_t textureOf(std::uint32_t image) const {
        return tiles_[images_[image].first];
    }

    kt::Resident view() {
        return kt::Resident { .catalog = kt::Catalog { .images = arrayOf(images_), .tiles = arrayOf(tiles_), .textures = arrayOf(textures_) },
            .texels = arrayOf(texels_),
            .starts = arrayOf(starts_) };
    }

 private:
    template <typename T>
    static v3d::moya::kernel::Array<T> arrayOf(std::vector<T> & values) {
        return v3d::moya::kernel::Array<T> { values.data(), static_cast<std::uint32_t>(values.size()) };
    }

    std::vector<kt::Image> images_;
    std::vector<std::uint32_t> tiles_;
    std::vector<kt::Texture> textures_;
    std::vector<glm::vec4> texels_;
    std::vector<std::uint32_t> starts_;
};

/** A footprint of no extent, as a point reads. **/
v3d::moya::kernel::surface::TextureFootprint point() {
    return {};
}

/** A checker of one texel squares, white at the top left, of a side. **/
std::vector<float> checker(std::uint32_t side) {
    std::vector<float> out;
    for (std::uint32_t row = 0; row < side; ++row) {
        for (std::uint32_t column = 0; column < side; ++column) {
            out.push_back(((row + column) % 2U) == 0U ? 1.0F : 0.0F);
        }
    }
    return out;
}

/** The values 0 to count - 1 over a count, so every texel differs. **/
std::vector<float> ramp(std::uint32_t count) {
    std::vector<float> out;
    out.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        out.push_back(static_cast<float>(i) / static_cast<float>(count));
    }
    return out;
}

/** A lookup's first channel, with a fallback of minus one. **/
template <kt::Texels X>
float read(const X & texels, std::uint32_t image, const glm::vec2 & uv, const v3d::moya::kernel::surface::TextureFootprint & footprint,
    const kt::Sampling & sampling) {
    return kt::lookup(texels, image, uv, footprint, sampling, glm::vec4 { -1.0F }).x;
}

/** Writes a scanline EXR file of one float channel, Y. **/
void writeY(const std::string & path, std::uint32_t width, std::uint32_t height, std::vector<float> values) {
    Imf::Header header(static_cast<int>(width), static_cast<int>(height));
    header.channels().insert("Y", Imf::Channel(Imf::FLOAT));
    Imf::FrameBuffer buffer;
    buffer.insert("Y", Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(values.data()), sizeof(float), sizeof(float) * width));
    Imf::OutputFile file(path.c_str(), header);
    file.setFrameBuffer(buffer);
    file.writePixels(static_cast<int>(height));
}

// a texel index beyond the edge reads the texel each address mode gives
BOOST_AUTO_TEST_CASE(moya_texture_lookup_address_modes_test) {
    BOOST_TEST(kt::addressed(5, 4, kt::Address::Periodic) == 1U);
    BOOST_TEST(kt::addressed(-1, 4, kt::Address::Periodic) == 3U);
    BOOST_TEST(kt::addressed(4, 4, kt::Address::Mirror) == 3U);
    BOOST_TEST(kt::addressed(-1, 4, kt::Address::Mirror) == 0U);
    BOOST_TEST(kt::addressed(9, 4, kt::Address::Mirror) == 1U);
    BOOST_TEST(kt::addressed(-7, 4, kt::Address::Clamp) == 0U);
    BOOST_TEST(kt::addressed(7, 4, kt::Address::Constant) == 3U);
}

// v runs up the texture, a texel's centre reads the texel, and Constant gives the default outside
BOOST_AUTO_TEST_CASE(moya_texture_lookup_reads_texels_where_materialx_places_them_test) {
    Held held;
    const std::uint32_t image = held.add(4, 2, ramp(8));
    const kt::Resident texels = held.view();
    const kt::Sampling clamped { .s = kt::Address::Clamp, .t = kt::Address::Clamp, .filtering = kt::Filtering::Linear };
    // the bottom row's second texel, and the top row's first
    BOOST_TEST(read(texels, image, glm::vec2 { 0.375F, 0.25F }, point(), clamped) == 5.0F / 8.0F);
    BOOST_TEST(read(texels, image, glm::vec2 { 0.125F, 0.75F }, point(), clamped) == 0.0F);
    // halfway between two texels of a row
    BOOST_TEST(read(texels, image, glm::vec2 { 0.25F, 0.75F }, point(), clamped) == 0.5F / 8.0F, boost::test_tools::tolerance(1.0e-6F));
    const kt::Sampling closest { .s = kt::Address::Clamp, .t = kt::Address::Clamp, .filtering = kt::Filtering::Closest };
    BOOST_TEST(read(texels, image, glm::vec2 { 0.26F, 0.74F }, point(), closest) == 1.0F / 8.0F);

    const kt::Sampling constant { .s = kt::Address::Constant, .t = kt::Address::Periodic, .filtering = kt::Filtering::Linear };
    BOOST_TEST(read(texels, image, glm::vec2 { 1.5F, 0.25F }, point(), constant) == -1.0F);
    BOOST_TEST(read(texels, image, glm::vec2 { 0.375F, 1.25F }, point(), constant) == 5.0F / 8.0F);
    BOOST_TEST(read(texels, 7U, glm::vec2 { 0.5F }, point(), constant) == -1.0F);
}

// a footprint many texels wide reads the coarser level that averages them
BOOST_AUTO_TEST_CASE(moya_texture_lookup_a_wide_footprint_reads_a_coarser_level_test) {
    Held held;
    const std::uint32_t image = held.add(16, 16, checker(16));
    const kt::Resident texels = held.view();
    const kt::Sampling sampling;
    const glm::vec2 centre { 0.5F / 16.0F, 1.0F - (0.5F / 16.0F) };
    // a point at the top left texel's centre reads its white
    BOOST_TEST(read(texels, image, centre, point(), sampling) == 1.0F);
    // a texel wide reads the first level, and sixteen wide the last, which averages them all
    const float texel = 1.0F / 16.0F;
    const v3d::moya::kernel::surface::TextureFootprint near { .major = glm::vec2 { texel, 0.0F }, .minor = glm::vec2 { 0.0F, texel } };
    const v3d::moya::kernel::surface::TextureFootprint far { .major = glm::vec2 { 1.0F, 0.0F }, .minor = glm::vec2 { 0.0F, 1.0F } };
    BOOST_TEST(read(texels, image, centre, near, sampling) == 1.0F);
    BOOST_TEST(read(texels, image, centre, far, sampling) == 0.5F, boost::test_tools::tolerance(1.0e-6F));
    // a footprint long along u and a texel across v probes along u, each probe at the first level
    const v3d::moya::kernel::surface::TextureFootprint streak { .major = glm::vec2 { 8.0F * texel, 0.0F }, .minor = glm::vec2 { 0.0F, texel } };
    BOOST_TEST(read(texels, image, glm::vec2 { 0.5F, 1.0F - (0.5F / 16.0F) }, streak, sampling) == 0.5F, boost::test_tools::tolerance(1.0e-6F));
}

// a UDIM image reads the tile its coordinate's whole parts name, and a gap gives the default
BOOST_AUTO_TEST_CASE(moya_texture_lookup_a_udim_tile_is_read_for_its_coordinates_test) {
    Held held;
    const std::uint32_t first = held.textureOf(held.add(1, 1, { 0.25F }));
    const std::uint32_t second = held.textureOf(held.add(1, 1, { 0.75F }));
    std::vector<std::uint32_t> tiles(20, kt::kNoTexture);
    tiles[0] = first;
    tiles[11] = second;
    const std::uint32_t image = held.grid(tiles, 2);
    const kt::Resident texels = held.view();
    const kt::Sampling sampling;
    BOOST_TEST(read(texels, image, glm::vec2 { 0.5F, 0.5F }, point(), sampling) == 0.25F);
    BOOST_TEST(read(texels, image, glm::vec2 { 1.5F, 1.5F }, point(), sampling) == 0.75F);
    BOOST_TEST(read(texels, image, glm::vec2 { 1.5F, 0.5F }, point(), sampling) == -1.0F);
    BOOST_TEST(read(texels, image, glm::vec2 { 0.5F, 2.5F }, point(), sampling) == -1.0F);
    BOOST_TEST(read(texels, image, glm::vec2 { -0.5F, 0.5F }, point(), sampling) == -1.0F);
}

// the CPU's texture access, through the cache, reads what the oracle holds, at every footprint
BOOST_AUTO_TEST_CASE(moya_texture_lookup_the_cpu_reads_what_the_oracle_holds_test) {
    const boost::filesystem::path directory = boost::filesystem::temp_directory_path() / "v3d_texture_lookup_cpu";
    boost::system::error_code ignored;
    boost::filesystem::remove_all(directory, ignored);
    boost::filesystem::create_directories(directory);
    constexpr std::uint32_t kSide = 150;
    const std::vector<float> values = ramp(kSide * kSide);
    const std::string picture = (directory / "ramp.exr").string();
    writeY(picture, kSide, kSide, values);

    Held held;
    const std::uint32_t image = held.add(kSide, kSide, values);
    const kt::Resident oracle = held.view();
    v3d::moya::device::CpuDevice device;
    texture::Options options;
    options.directory = (directory / "made").string();
    options.memory = 1;
    const texture::Textures made(device, { texture::Request { .file = picture, .colourSpace = "", .colour = false } }, options, v3d::moya::colour::Config(),
        boost::make_shared<v3d::log::Logger>());
    const v3d::moya::device::cpu::Textures cpu = made.view();
    BOOST_TEST_REQUIRE(cpu.catalog.textures.size == 1U);
    BOOST_TEST(cpu.catalog.textures[0].levels == oracle.catalog.textures[0].levels);

    const kt::Sampling sampling { .s = kt::Address::Mirror, .t = kt::Address::Periodic, .filtering = kt::Filtering::Linear };
    std::uint32_t compared = 0;
    for (const float width : { 0.0F, 0.004F, 0.03F, 0.2F }) {
        for (std::uint32_t i = 0; i < 40U; ++i) {
            const glm::vec2 uv { -0.3F + (0.041F * static_cast<float>(i)), 1.2F - (0.033F * static_cast<float>(i)) };
            const v3d::moya::kernel::surface::TextureFootprint footprint { .major = glm::vec2 { width, width * 0.5F }, .minor = glm::vec2 { -width * 0.1F, width * 0.2F } };
            BOOST_TEST(read(cpu, image, uv, footprint, sampling) == read(oracle, image, uv, footprint, sampling), "at " << uv.x << ", " << uv.y << " of " << width);
            ++compared;
        }
    }
    BOOST_TEST(compared == 160U);
    BOOST_TEST(made.cache().failures().empty());
    boost::filesystem::remove_all(directory, ignored);
}

};  // namespace
