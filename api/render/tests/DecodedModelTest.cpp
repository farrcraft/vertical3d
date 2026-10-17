/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/image/Factory.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/DecodedModel.h>

#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem.hpp>
#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

using v3d::render::realtime::DecodedModel;
using v3d::render::realtime::decodeAlbedos;
using v3d::render::realtime::decodeModel;

namespace {

/**
 * A directory holding a copy of three_primitives.glb under models/, which names models/albedo.png.
 * The picture is written beside it only when asked for.
 **/
boost::shared_ptr<v3d::asset::Manager> root(const std::string& name, bool withAlbedo) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::filesystem::path directory = boost::filesystem::path("data_out") / name;
    boost::filesystem::remove_all(directory);
    boost::filesystem::create_directories(directory / "models");
    boost::filesystem::copy_file(boost::filesystem::path(V3D_ASSET_FIXTURES) / "three_primitives.glb",
        directory / "models" / "three_primitives.glb");
    boost::filesystem::copy_file(boost::filesystem::path(V3D_ASSET_FIXTURES) / "embedded_texture.glb",
        directory / "embedded_texture.glb");
    if (withAlbedo) {
        const boost::shared_ptr<v3d::image::Image> pixel = boost::make_shared<v3d::image::Image>(2, 2, 24);
        v3d::image::Factory factory(logger);
        BOOST_REQUIRE(factory.write((directory / "models" / "albedo.png").string(), pixel));
    }

    boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>(directory.string(), logger);
    v3d::asset::media::registerLoaders(*assets, logger);
    return assets;
}

};  // namespace

/**
 * A texture the file names is decoded from beside the file, and keyed by that path.
 **/
BOOST_AUTO_TEST_CASE(a_named_albedo_is_decoded_beside_its_model) {
    const boost::shared_ptr<v3d::asset::Manager> assets = root("decoded_named", true);

    const DecodedModel decoded = decodeModel(*assets, "models/three_primitives.glb");

    BOOST_CHECK_EQUAL(decoded.path, "models/three_primitives.glb");
    BOOST_REQUIRE(decoded.model);
    BOOST_REQUIRE_EQUAL(decoded.albedos.size(), decoded.model->materials().size());
    BOOST_CHECK_EQUAL(decoded.albedos[0].key, "models/albedo.png");
    BOOST_REQUIRE(decoded.albedos[0].pixels);
    BOOST_CHECK_EQUAL(decoded.albedos[0].pixels->width(), 2U);
}

/**
 * A texture the file embeds keeps the pixels the loader decoded, keyed by the file.
 **/
BOOST_AUTO_TEST_CASE(an_embedded_albedo_keeps_its_pixels) {
    const boost::shared_ptr<v3d::asset::Manager> assets = root("decoded_embedded", false);

    const DecodedModel decoded = decodeModel(*assets, "embedded_texture.glb");

    BOOST_REQUIRE(!decoded.albedos.empty());
    BOOST_CHECK_EQUAL(decoded.albedos[0].key, "embedded_texture.glb#albedo0");
    BOOST_CHECK(decoded.albedos[0].pixels);
}

/**
 * A named texture that is not there keeps its key and has no pixels, so the model still decodes.
 **/
BOOST_AUTO_TEST_CASE(a_missing_albedo_has_a_key_and_no_pixels) {
    const boost::shared_ptr<v3d::asset::Manager> assets = root("decoded_missing", false);

    const DecodedModel decoded = decodeModel(*assets, "models/three_primitives.glb");

    BOOST_REQUIRE(!decoded.albedos.empty());
    BOOST_CHECK_EQUAL(decoded.albedos[0].key, "models/albedo.png");
    BOOST_CHECK(!decoded.albedos[0].pixels);
}

/**
 * An albedo already uploaded is not decoded again.
 **/
BOOST_AUTO_TEST_CASE(an_uploaded_albedo_is_not_decoded) {
    const boost::shared_ptr<v3d::asset::Manager> assets = root("decoded_uploaded", true);

    const DecodedModel decoded = decodeModel(*assets, "models/three_primitives.glb",
        [](const std::string& key) { return key == "models/albedo.png"; });

    BOOST_REQUIRE(!decoded.albedos.empty());
    BOOST_CHECK_EQUAL(decoded.albedos[0].key, "models/albedo.png");
    BOOST_CHECK(!decoded.albedos[0].pixels);
}

/**
 * Two materials naming one image share one decoded image.
 **/
BOOST_AUTO_TEST_CASE(two_albedos_with_one_key_share_one_image) {
    const boost::shared_ptr<v3d::asset::Manager> assets = root("decoded_shared", true);
    std::vector<DecodedModel::Albedo> albedos(2);
    albedos[0].key = "models/albedo.png";
    albedos[1].key = "models/albedo.png";

    decodeAlbedos(*assets, &albedos);

    BOOST_REQUIRE(albedos[0].pixels);
    BOOST_CHECK(albedos[0].pixels == albedos[1].pixels);
}

/**
 * A file that is not a model is refused, as MeshRegistry::load refuses it.
 **/
BOOST_AUTO_TEST_CASE(a_file_that_is_not_a_model_is_refused) {
    const boost::shared_ptr<v3d::asset::Manager> assets = root("decoded_refused", true);

    BOOST_CHECK_THROW(decodeModel(*assets, "models/albedo.png"), std::runtime_error);
}
