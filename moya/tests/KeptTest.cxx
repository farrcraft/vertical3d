/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/log/Logger.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/cpu/Textures.h>
#include <moya/libmoya/geometry/Cache.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/kernel/texture/Texture.h>
#include <moya/libmoya/kernel/volume/Block.h>
#include <moya/libmoya/scene/Change.h>
#include <moya/libmoya/scene/Handle.h>
#include <moya/libmoya/scene/Kinds.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/scene/Value.h>
#include <moya/libmoya/texture/Cache.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Request.h>
#include <moya/libmoya/texture/Textures.h>
#include <moya/libmoya/volume/Grid.h>
#include <moya/libmoya/volume/Volumes.h>

#include <OpenEXR/ImfChannelList.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfOutputFile.h>

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <set>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec3.hpp>

#include "Staged.h"

namespace v3d::moya::tracing {
namespace {

namespace geometry = v3d::moya::geometry;
namespace scene = v3d::moya::scene;
namespace texture = v3d::moya::texture;
namespace volume = v3d::moya::volume;
typedef v3d::moya::device::CpuDevice CpuDevice;
typedef std::set<std::uint32_t> Indices;

/**
 * A staged scene of two quads of their own OpenPBR materials, red and blue, and a sphere of fog
 * whose VDF reaches its volume node through a connection. The cache has gathered it once.
 **/
struct Shaded {
    Staged staged { 1 };
    scene::Handle red;
    scene::Handle blue;
    scene::Handle fog;
    scene::Handle vdf;
    geometry::Cache cache;
    boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    std::uint64_t version { 0 };

    Shaded() {
        red = staged.material("red", { { "base_color", scene::Value::colour(glm::vec3 { 0.8F, 0.1F, 0.1F }) } });
        blue = staged.material("blue", { { "base_color", scene::Value::colour(glm::vec3 { 0.1F, 0.1F, 0.8F }) } });
        staged.quad("left", { { -1.0F, -1.0F, 0.0F }, { 0.0F, -1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F }, { -1.0F, 1.0F, 0.0F } }, red);
        staged.quad("right", { { 0.0F, -1.0F, 0.0F }, { 1.0F, -1.0F, 0.0F }, { 1.0F, 1.0F, 0.0F }, { 0.0F, 1.0F, 0.0F } }, blue);
        staged.fog("fog", 0.5F, glm::vec3 { 0.0F, 0.0F, 1.0F }, glm::vec3 { 0.1F }, glm::vec3 { 0.5F }, 0.0F);
        staged.made.commit();
        version = staged.made.apply();
        // an object is found by its name once it is applied
        fog = *staged.made.find("material", "fog");
        vdf = *staged.made.find("shader", "fog vdf");
        geometry::gather(staged.made, logger, cache);
    }

    /**
     * Commits and applies what a case changed, tells the cache of every change, and gives the
     * gather through the cache.
     **/
    geometry::Prototypes regathered() {
        staged.made.commit();
        const std::uint64_t applied = staged.made.apply();
        for (const scene::Change & change : staged.made.changes(version)) {
            cache.forget(change.handle.index);
        }
        version = applied;
        return geometry::gather(staged.made, logger, cache);
    }

    /** The gather of the scene as it now stands, of no cache. **/
    geometry::Prototypes fresh() const {
        return geometry::gather(staged.made, logger);
    }
};

/** Whether two vectors hold the same bytes. **/
template <typename T>
bool sameBytes(const std::vector<T> & one, const std::vector<T> & other) {
    return one.size() == other.size() && (one.empty() || std::memcmp(one.data(), other.data(), one.size() * sizeof(T)) == 0);
}

/** Whether two gathers give the same materials, programs, images and primvar names. **/
bool samePrograms(const geometry::Prototypes & one, const geometry::Prototypes & other) {
    return sameBytes(one.materials, other.materials) && sameBytes(one.instructions, other.instructions) && sameBytes(one.programValues, other.programValues) &&
        sameBytes(one.programColours, other.programColours) && one.programSpaces == other.programSpaces && one.images == other.images &&
        one.primvarNames == other.primvarNames && one.materialSpaces == other.materialSpaces;
}

/** Writes a scanline EXR file of one float channel, Y, of four by four texels of one value. **/
std::string writtenY(const std::string & name, float value) {
    const boost::filesystem::path directory = boost::filesystem::temp_directory_path() / "v3d_kept";
    boost::filesystem::create_directories(directory);
    const std::string path = (directory / name).string();
    std::vector<float> values(16, value);
    Imf::Header header(4, 4);
    header.channels().insert("Y", Imf::Channel(Imf::FLOAT));
    Imf::FrameBuffer buffer;
    buffer.insert("Y", Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(values.data()), sizeof(float), sizeof(float) * 4));
    Imf::OutputFile file(path.c_str(), header);
    file.setFrameBuffer(buffer);
    file.writePixels(4);
    return path;
}

/** The options of the textures the cases make, which are made in a directory of their own. **/
texture::Options madeIn() {
    texture::Options out;
    out.directory = (boost::filesystem::temp_directory_path() / "v3d_kept" / "made").string();
    return out;
}

/** A request for an image read as data. **/
texture::Request data(const std::string & file) {
    return texture::Request { .file = file, .colourSpace = "", .colour = false };
}

/** The texture each image reads, by its first tile, as the textures' view holds them. **/
std::vector<v3d::moya::kernel::texture::Texture> texturesRead(const texture::Textures & textures) {
    const v3d::moya::device::cpu::Textures view = textures.view();
    std::vector<v3d::moya::kernel::texture::Texture> out;
    for (std::uint32_t image = 0; image < view.catalog.images.size; ++image) {
        out.push_back(view.catalog.textures[view.catalog.tiles[view.catalog.images[image].first]]);
    }
    return out;
}

/** A grid of a number of blocks, each word of which holds a value. **/
boost::shared_ptr<const volume::Grid> gridOf(std::size_t blocks, std::uint64_t value) {
    auto out = boost::make_shared<volume::Grid>();
    out->blocks.resize(blocks);
    for (v3d::moya::kernel::volume::Block & block : out->blocks) {
        for (std::uint64_t & word : block.words) {
            word = value;
        }
    }
    return out;
}

/** The blocks a view of volumes holds, in order. **/
std::vector<std::uint64_t> wordsOf(const volume::Volumes & volumes) {
    const v3d::moya::kernel::volume::Volumes view = volumes.view();
    std::vector<std::uint64_t> out;
    for (std::uint32_t block = 0; block < view.blocks.size; ++block) {
        for (const std::uint64_t word : view.blocks[block].words) {
            out.push_back(word);
        }
    }
    return out;
}

// a change to one material's input compiles that material alone, and the gather is the one a
// gather of no cache gives
BOOST_AUTO_TEST_CASE(moya_kept_a_changed_material_alone_is_compiled_again_test) {
    Shaded shaded;
    BOOST_TEST(shaded.cache.compiled() == (Indices { shaded.red.index, shaded.blue.index, shaded.fog.index }));
    const scene::Handle surface = shaded.staged.made.value(shaded.red, "surface").handleArray().front();
    shaded.staged.made.set(surface, "base_color", scene::Value::colour(glm::vec3 { 0.2F, 0.7F, 0.2F }));
    const geometry::Prototypes kept = shaded.regathered();
    BOOST_TEST(shaded.cache.compiled() == Indices { shaded.red.index });
    BOOST_TEST(samePrograms(kept, shaded.fresh()));
}

// a change to a node that feeds a volume through a connection compiles that material alone
BOOST_AUTO_TEST_CASE(moya_kept_a_node_feeding_a_material_compiles_it_again_test) {
    Shaded shaded;
    shaded.staged.made.set(shaded.vdf, "anisotropy", scene::Value::number(0.5F));
    const geometry::Prototypes kept = shaded.regathered();
    BOOST_TEST(shaded.cache.compiled() == Indices { shaded.fog.index });
    BOOST_TEST(samePrograms(kept, shaded.fresh()));
}

// a removed connection is a change to the network it was part of
BOOST_AUTO_TEST_CASE(moya_kept_a_removed_connection_compiles_its_material_again_test) {
    Shaded shaded;
    shaded.staged.made.remove(*shaded.staged.made.find("connection", "fog into volume"));
    const geometry::Prototypes kept = shaded.regathered();
    BOOST_TEST(shaded.cache.compiled() == Indices { shaded.fog.index });
    BOOST_TEST(samePrograms(kept, shaded.fresh()));
}

// a change to the camera, or to an option but the shader search path, compiles no material
BOOST_AUTO_TEST_CASE(moya_kept_a_camera_or_an_option_compiles_nothing_test) {
    Shaded shaded;
    const scene::Handle camera = shaded.staged.made.value(shaded.staged.options, "camera").handleArray().front();
    shaded.staged.made.set(camera, "fov", scene::Value::number(40.0F));
    shaded.staged.made.set(shaded.staged.options, "samples", scene::Value::integer(16));
    const geometry::Prototypes kept = shaded.regathered();
    BOOST_TEST(shaded.cache.compiled().empty());
    BOOST_TEST(samePrograms(kept, shaded.fresh()));
}

// a new shader search path compiles every material again
BOOST_AUTO_TEST_CASE(moya_kept_a_shader_search_path_compiles_every_material_test) {
    Shaded shaded;
    shaded.staged.made.set(shaded.staged.options, "shaderSearchPath", scene::Value::string(boost::filesystem::temp_directory_path().string()));
    const geometry::Prototypes kept = shaded.regathered();
    BOOST_TEST(shaded.cache.compiled() == (Indices { shaded.red.index, shaded.blue.index, shaded.fog.index }));
    BOOST_TEST(samePrograms(kept, shaded.fresh()));
}

// a material removed and another made leave the gather the one of no cache
BOOST_AUTO_TEST_CASE(moya_kept_a_removed_and_a_made_material_test) {
    Shaded shaded;
    shaded.staged.made.remove(shaded.blue);
    const scene::Handle green = shaded.staged.material("green", { { "base_color", scene::Value::colour(glm::vec3 { 0.1F, 0.8F, 0.1F }) } });
    shaded.staged.made.set(*shaded.staged.made.find("instance", "right"), "material", scene::Value::handle(green));
    const geometry::Prototypes kept = shaded.regathered();
    BOOST_TEST(shaded.cache.compiled() == Indices { green.index });
    BOOST_TEST(samePrograms(kept, shaded.fresh()));
}

// an update opens only the images not read before, closes each no image reads any more, and
// reads every image as textures made from nothing do
BOOST_AUTO_TEST_CASE(moya_kept_textures_open_the_new_and_close_the_unread_test) {
    const std::string one = writtenY("one.exr", 0.25F);
    const std::string two = writtenY("two.exr", 0.5F);
    const std::string three = writtenY("three.exr", 0.75F);
    CpuDevice device;
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    texture::Textures kept(device, { data(one), data(two) }, madeIn(), v3d::moya::colour::Config(), logger);
    BOOST_TEST(kept.cache().opened() == 2U);
    const std::uint32_t second = kept.view().catalog.tiles[1];
    float texel[4] = { 1.0F, 1.0F, 1.0F, 1.0F };
    texture::Cache::fetch(kept.view().cache, second, 0, 0, 0, texel);
    BOOST_TEST(texel[0] == 0.5F);

    kept.update(device, { data(three), data(one) });
    BOOST_TEST(kept.cache().opened() == 2U);
    // one is kept, three is opened, and two is closed and reads as nothing
    BOOST_TEST(kept.cache().textures().size() == 3U);
    texture::Cache::fetch(kept.view().cache, second, 0, 0, 0, texel);
    BOOST_TEST(texel[0] == 0.0F);
    const texture::Textures built(device, { data(three), data(one) }, madeIn(), v3d::moya::colour::Config(), logger);
    BOOST_TEST(sameBytes(texturesRead(kept), texturesRead(built)));
    boost::system::error_code ignored;
    boost::filesystem::remove_all(boost::filesystem::temp_directory_path() / "v3d_kept", ignored);
}

// volumes lay their blocks out again only when their grids change, and hold the blocks volumes
// made from nothing hold
BOOST_AUTO_TEST_CASE(moya_kept_volumes_lay_out_again_only_for_new_grids_test) {
    CpuDevice device;
    geometry::Prototypes prototypes;
    prototypes.grids = { gridOf(2, 1U), gridOf(3, 2U) };
    volume::Volumes kept(device, prototypes);
    BOOST_TEST(kept.laidOut());
    kept.update(device, prototypes);
    BOOST_TEST(!kept.laidOut());
    prototypes.grids[1] = gridOf(1, 3U);
    kept.update(device, prototypes);
    BOOST_TEST(kept.laidOut());
    BOOST_TEST(wordsOf(kept) == wordsOf(volume::Volumes(device, prototypes)), boost::test_tools::per_element());
}

};  // namespace
};  // namespace v3d::moya::tracing
