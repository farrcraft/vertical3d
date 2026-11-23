/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/asset/Writer.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/rib/Reader.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Textures.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/program/Instruction.h>
#include <moya/libmoya/kernel/program/Op.h>
#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/material/Compile.h>
#include <moya/libmoya/material/Library.h>
#include <moya/libmoya/ri/SceneClient.h>
#include <moya/libmoya/scene/Declaration.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/texture/Cache.h>
#include <moya/libmoya/texture/Maker.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Request.h>
#include <moya/libmoya/texture/SearchPath.h>
#include <moya/libmoya/texture/Textures.h>
#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfOutputFile.h>
#include <OpenEXR/ImfPixelType.h>

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <ctime>
#include <exception>
#include <optional>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Tracing.h"

namespace {

namespace scene = v3d::moya::scene;
namespace texture = v3d::moya::texture;
using v3d::moya::tracing::instanced;
using v3d::moya::tracing::moved;

constexpr const char * kOpenPbr = "ND_open_pbr_surface_surfaceshader";

/**
 * A directory of its own per case, removed with everything under it, so a case that leaves a file
 * behind fails the case that looks for it rather than the next run.
 **/
class Sandbox {
 public:
    explicit Sandbox(const std::string & name) : path_(boost::filesystem::temp_directory_path() / ("v3d_texture_cache_" + name)) {
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

    std::string path() const {
        return path_.string();
    }

 private:
    boost::filesystem::path path_;
};

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

/** The values 0 to count - 1 over a count, so every texel differs. **/
std::vector<float> ramp(std::uint32_t count) {
    std::vector<float> out;
    out.reserve(count);
    for (std::uint32_t i = 0; i < count; ++i) {
        out.push_back(static_cast<float>(i) / static_cast<float>(count));
    }
    return out;
}

boost::shared_ptr<v3d::log::Logger> logOf() {
    return boost::make_shared<v3d::log::Logger>();
}

/** How many regular files a directory holds. **/
std::size_t filesIn(const std::string & directory) {
    boost::system::error_code ignored;
    if (!boost::filesystem::is_directory(directory, ignored)) {
        return 0;
    }
    return static_cast<std::size_t>(std::count_if(boost::filesystem::directory_iterator(directory), boost::filesystem::directory_iterator(),
        [](const boost::filesystem::directory_entry & entry) { return boost::filesystem::is_regular_file(entry.path()); }));
}

/** Every texel of every level of a texture, each tile's in turn, through a cache. **/
std::vector<float> everyTexel(texture::Cache & cache, std::uint32_t texture) {
    const v3d::moya::kernel::texture::Texture & held = cache.textures().at(texture);
    std::vector<float> out;
    for (std::uint32_t level = 0; level < held.levels; ++level) {
        // each row forwards and then backwards, so a small cache drops tiles it reads again
        for (const bool back : { false, true }) {
            for (std::uint32_t row = 0; row < held.heightAt(level); ++row) {
                for (std::uint32_t column = 0; column < held.widthAt(level); ++column) {
                    float texel[4] {};
                    const std::uint32_t at = back ? held.widthAt(level) - 1U - column : column;
                    texture::Cache::fetch(&cache, texture, level, at, row, texel);
                    out.insert(out.end(), texel, texel + 4);
                }
            }
        }
    }
    return out;
}

/** A shader node of a definition with inputs. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the node's name, then its definition
scene::Handle node(scene::Scene & scene, const std::string & name, const std::string & definition, const std::vector<std::pair<std::string, scene::Value>> & inputs) {
    const scene::Handle out = scene.create("shader", name);
    scene.set(out, "shader", scene::Value::string(definition));
    for (const auto & [input, value] : inputs) {
        scene.declare(out, scene::Declaration { .name = input, .type = value.type(), .count = static_cast<std::uint32_t>(value.size()), .fallback = value });
        scene.set(out, input, value);
    }
    return out;
}

void connect(scene::Scene & scene, const scene::Handle & from, const scene::Handle & to, const std::string & input) {
    const scene::Handle out = scene.create("connection", std::to_string(from.index) + " to " + input + " of " + std::to_string(to.index));
    scene.set(out, "from", scene::Value::handle(from));
    scene.set(out, "to", scene::Value::handle(to));
    scene.set(out, "input", scene::Value::string(input));
}

/** A material whose base colour is an image, read through an image node of a definition. **/
scene::Handle imaged(scene::Scene & scene, const std::string & file, const std::string & definition) {
    const scene::Handle read = node(scene, "read", definition, { { "file", scene::Value::string(file) } });
    const scene::Handle surface = node(scene, "surface", kOpenPbr, { { "specular_weight", scene::Value::number(0.0F) } });
    connect(scene, read, surface, "base_color");
    const scene::Handle out = scene.create("material", "imaged");
    scene.set(out, "surface", scene::Value::handle(surface));
    return out;
}

/**
 * A square from -1 to 1 in z = 0, facing a camera five units back along -z, under a white dome, at
 * 16 pixels square. Its st runs from zero at its bottom left to one at its top right.
 **/
void framedSquare(scene::Scene & scene, const scene::Handle & material, std::int32_t memory) {
    const scene::Handle options = scene.create("options", "options");
    const scene::Handle camera = scene.create("camera", "camera");
    scene.set(options, "camera", scene::Value::handle(camera));
    scene.set(options, "resolution", scene::Value::integers({ 16, 16 }));
    scene.set(options, "samples", scene::Value::integer(4));
    scene.set(options, "textureMemory", scene::Value::integer(memory));
    scene.create("domeLight", "sky");
    scene.set(camera, "fov", scene::Value::number(30.0F));
    scene.set(camera, "transform", scene::Value::matrix(moved(0.0F, 0.0F, -5.0F)));
    const scene::Handle square = scene.create("mesh", "square");
    scene.set(square, "P", scene::Value::points({ { -1.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }));
    scene.set(square, "vertices", scene::Value::integers({ 3, 3 }));
    scene.set(square, "indices", scene::Value::integers({ 0, 2, 1, 0, 3, 2 }));
    const scene::Value st = scene::Value::floats({ 0.0F, 0.0F, 1.0F, 0.0F, 1.0F, 1.0F, 0.0F, 1.0F });
    scene.declare(square,
        scene::Declaration { .name = "st", .type = st.type(), .count = scene::Declaration::kAny, .width = 2, .fallback = st, .interpolation = scene::Interpolation::Vertex });
    scene.set(square, "st", st);
    const scene::Handle placed = instanced(scene, "placed", square, {}, glm::mat4(1.0F));
    if (material.valid()) {
        scene.set(placed, "material", scene::Value::handle(material));
    }
    scene.commit();
    scene.apply();
}

/** The beauty a square textured by a file renders, with a texture memory in kilobytes. **/
std::vector<float> squareOf(const std::string & file, std::int32_t memory) {
    scene::Scene held(scene::kinds());
    framedSquare(held, file.empty() ? scene::Handle {} : imaged(held, file, "ND_image_color3"), memory);
    v3d::moya::device::CpuDevice device;
    return v3d::moya::integrate::render(device, held, logOf(), v3d::moya::colour::Config()).values;
}

/** An option of the scene a RIB text gives through the scene client, as text. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the RIB, then the option's name
std::string optionOf(const std::string & source, const std::string & name) {
    scene::Scene held(scene::kinds());
    const auto logger = logOf();
    v3d::moya::ri::SceneClient client(held, logger);
    v3d::rib::Reader reader(logger);
    std::istringstream stream(source);
    BOOST_REQUIRE(reader.read(stream, &client));
    held.commit();
    held.apply();
    const scene::Value & value = held.value(*held.find("options", "options"), name);
    return value.type() == scene::Type::String ? value.stringArray().front() : std::to_string(value.integerArray().front());
}

// a cache of one tile reads every texel as a cache of every tile does, loading tiles again
BOOST_AUTO_TEST_CASE(moya_texture_cache_of_one_tile_reads_what_any_cache_reads_test) {
    const Sandbox sandbox("one_tile");
    const std::string picture = sandbox.file("ramp.exr");
    const std::string made = sandbox.file("ramp.tx");
    writeY(picture, 150, 90, ramp(150 * 90));
    texture::Making data;
    data.data = true;
    texture::make(picture, made, data, v3d::moya::colour::Config(), logOf());

    texture::Cache ample(std::size_t { 1 } << 30U);
    texture::Cache small(1);
    const std::uint32_t inAmple = ample.open(made);
    const std::uint32_t inSmall = small.open(made);
    BOOST_TEST(everyTexel(ample, inAmple) == everyTexel(small, inSmall));
    // three by two tiles at the first level, two by one at the second, and one at each other
    const std::uint64_t tiles = 6U + 2U + 6U;
    BOOST_TEST(ample.loads() == tiles);
    BOOST_TEST(small.loads() > tiles);
    BOOST_TEST(ample.failures().empty());
    BOOST_TEST(small.failures().empty());

    // a file that is not tiled is refused
    BOOST_CHECK_THROW(ample.open(picture), std::exception);
}

// a search path's directories are separated by colons and semicolons, but a drive's colon is kept
BOOST_AUTO_TEST_CASE(moya_texture_search_path_test) {
    BOOST_TEST(texture::directoriesOf("maps:more;other") == std::vector<std::string>({ "maps", "more", "other" }), boost::test_tools::per_element());
    BOOST_TEST(texture::directoriesOf("C:/maps:D:/more;;") == std::vector<std::string>({ "C:/maps", "D:/more" }), boost::test_tools::per_element());
    BOOST_TEST(texture::directoriesOf("").empty());

    const Sandbox sandbox("search");
    boost::filesystem::create_directories(sandbox.file("second"));
    writeY(sandbox.file("second/found.exr"), 1, 1, { 1.0F });
    const std::optional<std::string> found = texture::found("found.exr", { sandbox.file("first"), sandbox.file("second") });
    BOOST_REQUIRE(found.has_value());
    BOOST_TEST(boost::filesystem::equivalent(*found, sandbox.file("second/found.exr")));
    BOOST_TEST(!texture::found("absent.exr", { sandbox.path() }).has_value());
    BOOST_TEST(texture::found(sandbox.file("second/found.exr"), {}).has_value());
}

// an image that is not a texture is made into one once, named by its content, and read again
BOOST_AUTO_TEST_CASE(moya_texture_an_image_is_made_once_and_read_again_test) {
    const Sandbox sandbox("made");
    const std::string picture = sandbox.file("grey.png");
    const auto image = boost::make_shared<v3d::image::Image>(glm::uvec2(2, 2), 24);
    std::fill(image->data(), image->data() + 12, static_cast<unsigned char>(128));
    v3d::image::Factory factory(logOf());
    BOOST_REQUIRE(v3d::asset::writeFile(picture, *factory.encode(image, "png")));

    texture::Options options;
    options.directory = sandbox.file("made");
    options.searchPath = { sandbox.path() };
    v3d::moya::device::CpuDevice device;
    const std::vector<texture::Request> requests { texture::Request { .file = "grey.png", .colourSpace = "", .colour = true },
        texture::Request { .file = "grey.png", .colourSpace = "", .colour = false }, texture::Request { .file = "grey.png", .colourSpace = "", .colour = true } };
    {
        const texture::Textures first(device, requests, options, v3d::moya::colour::Config(), logOf());
        // the colour and data readings are two textures, and the third request shares the first's
        BOOST_TEST(first.cache().textures().size() == 2U);
        const v3d::moya::device::cpu::Textures view = first.view();
        BOOST_TEST(view.catalog.tiles[0] == view.catalog.tiles[2]);
    }
    BOOST_TEST(filesIn(options.directory) == 2U);
    const boost::filesystem::directory_iterator kept(options.directory);
    const std::time_t written = boost::filesystem::last_write_time(kept->path());
    const texture::Textures second(device, requests, options, v3d::moya::colour::Config(), logOf());
    BOOST_TEST(filesIn(options.directory) == 2U);
    BOOST_TEST(boost::filesystem::last_write_time(kept->path()) == written);

    // a file not found reads as nothing
    const texture::Textures absent(device, { texture::Request { .file = "absent.png", .colourSpace = "", .colour = true } }, options, v3d::moya::colour::Config(), logOf());
    BOOST_TEST(absent.view().catalog.tiles[0] == v3d::moya::kernel::texture::kNoTexture);
}

// a UDIM image's tiles are found by their numbers, ten to a row of v
BOOST_AUTO_TEST_CASE(moya_texture_a_udim_image_finds_its_tiles_test) {
    const Sandbox sandbox("udim");
    writeY(sandbox.file("paint.1001.exr"), 1, 1, { 0.25F });
    writeY(sandbox.file("paint.1012.exr"), 1, 1, { 0.75F });
    texture::Options options;
    options.directory = sandbox.file("made");
    v3d::moya::device::CpuDevice device;
    const texture::Textures held(device, { texture::Request { .file = sandbox.file("paint.<UDIM>.exr"), .colourSpace = "", .colour = false } }, options,
        v3d::moya::colour::Config(), logOf());
    const v3d::moya::device::cpu::Textures view = held.view();
    BOOST_REQUIRE(view.catalog.images.size == 1U);
    BOOST_TEST(view.catalog.images[0].columns == 10U);
    BOOST_TEST(view.catalog.images[0].rows == 2U);
    BOOST_TEST(view.catalog.tiles.size == 20U);
    BOOST_TEST(view.catalog.tiles[0] != v3d::moya::kernel::texture::kNoTexture);
    BOOST_TEST(view.catalog.tiles[1] == v3d::moya::kernel::texture::kNoTexture);
    BOOST_TEST(view.catalog.tiles[11] != v3d::moya::kernel::texture::kNoTexture);
    float texel[4] {};
    view.fetch(view.cache, view.catalog.tiles[11], 0, 0, 0, texel);
    BOOST_TEST(texel[0] == 0.75F);
}

// RI's texture search path and memory limit reach the options, & standing for the path before
BOOST_AUTO_TEST_CASE(moya_texture_rib_sets_the_search_path_and_memory_test) {
    BOOST_TEST(optionOf("Option \"searchpath\" \"string texture\" \"a:b\"\nOption \"searchpath\" \"string texture\" \"c:&\"\n", "textureSearchPath") == "c:a:b");
    BOOST_TEST(optionOf("Option \"limits\" \"integer texturememory\" [2048]\n", "textureMemory") == "2048");
    BOOST_TEST(optionOf("Option \"limits\" \"integer texturememory\" [0]\n", "textureMemory") == "1048576");
}

// an image node compiles to an image instruction, and a tiled image flattens to one
BOOST_AUTO_TEST_CASE(moya_texture_an_image_node_compiles_to_a_lookup_test) {
    for (const std::string definition : { "ND_image_color3", "ND_tiledimage_color3" }) {
        scene::Scene held(scene::kinds());
        const scene::Handle material = imaged(held, "paint.png", definition);
        held.commit();
        held.apply();
        std::vector<std::string> primvars { "N", "Cs", "tangent", "st" };
        const v3d::moya::material::Compiled compiled = v3d::moya::material::compile(v3d::moya::material::Library::installed(), held, material, primvars);
        BOOST_TEST_REQUIRE(compiled.images.size() == 1U, definition);
        BOOST_TEST(compiled.images.front().file == "paint.png");
        BOOST_TEST(compiled.images.front().colour);
        const bool reads = std::ranges::any_of(compiled.instructions, [](const v3d::moya::kernel::program::Instruction & instruction) {
            return instruction.op == v3d::moya::kernel::program::Op::Image && instruction.in[2] == 0U;
        });
        BOOST_TEST(reads, definition);
    }
}

// a render with a cache of one tile gives the picture a render with room for every tile gives
BOOST_AUTO_TEST_CASE(moya_texture_a_render_with_one_tile_matches_one_with_every_tile_test) {
    const Sandbox sandbox("render");
    constexpr std::uint32_t kSide = 200;
    std::vector<float> stripes;
    for (std::uint32_t row = 0; row < kSide; ++row) {
        for (std::uint32_t column = 0; column < kSide; ++column) {
            stripes.push_back((column / 25U) % 2U == 0U ? 0.9F : 0.1F);
        }
    }
    const std::string picture = sandbox.file("stripes.exr");
    writeY(picture, kSide, kSide, stripes);
    const std::vector<float> ample = squareOf(picture, 1048576);
    const std::vector<float> small = squareOf(picture, 1);
    BOOST_TEST(ample == small);
    // the texture is read: the square differs from one of no material
    BOOST_TEST(ample != squareOf("", 1048576));
}

};  // namespace
