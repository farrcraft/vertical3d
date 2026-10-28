/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/media/Loaders.h>
#include <api/asset/media/kind/Image.h>
#include <api/asset/media/kind/Model.h>
#include <api/log/Logger.h>

#include <typeinfo>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

boost::shared_ptr<v3d::asset::Manager> manager() {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>("data", logger);
    v3d::asset::media::registerLoaders(*assets, logger);
    return assets;
}

/**
 * Once the media loaders are registered, a manager loads a picture or a model by its
 * extension. A manager without them loads only documents.
 **/
BOOST_AUTO_TEST_CASE(media_registered_by_extension_test) {
    auto assets = manager();

    auto picture = assets->load("pixel.png");
    BOOST_TEST(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::media::kind::Image>(picture)));

    auto model = assets->load("two_surfaces.glb");
    BOOST_TEST(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::media::kind::Model>(model)));

    auto images = assets->resolveLoader<v3d::asset::media::kind::Image>();
    BOOST_REQUIRE(images);
    BOOST_TEST((images->kind() == typeid(v3d::asset::media::kind::Image)));
    auto models = assets->resolveLoader<v3d::asset::media::kind::Model>();
    BOOST_REQUIRE(models);
    BOOST_TEST((models->kind() == typeid(v3d::asset::media::kind::Model)));
}

/**
 * A picture that is not there, or a file of the wrong format, is no asset at all.
 **/
BOOST_AUTO_TEST_CASE(media_missing_image_test) {
    auto assets = manager();

    BOOST_TEST(!assets->load("absent.png"));
    BOOST_TEST(!assets->load<v3d::asset::media::kind::Image>("document.json"));
}

/**
 * A manager the media loaders were not registered on loads documents and nothing else: a
 * picture or a model by its extension is no asset.
 **/
BOOST_AUTO_TEST_CASE(media_unregistered_manager_loads_only_documents_test) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    v3d::asset::Manager plain("data", logger);

    BOOST_TEST(!plain.load("pixel.png"));
    BOOST_TEST(!plain.load("two_surfaces.glb"));
    BOOST_TEST(static_cast<bool>(plain.load("document.json")));
}

};  // namespace
