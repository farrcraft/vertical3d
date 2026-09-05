/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <stdexcept>
#include <string>

#include <boost/test/unit_test.hpp>
#include <boost/make_shared.hpp>

#include "../Image.h"
#include "../Json.h"
#include "../Manager.h"
#include "../Sound.h"
#include "../Text.h"
#include "../Type.h"

namespace {

boost::shared_ptr<v3d::asset::Manager> manager(const std::string& path = "data") {
    return boost::make_shared<v3d::asset::Manager>(path, boost::make_shared<v3d::log::Logger>());
}

};  // namespace

/**
 * Every type the constructor registers has to come back, because loadTypeFromExt maps an
 * extension straight onto one of them and resolveLoader throws for anything else.
 **/
BOOST_AUTO_TEST_CASE(manager_loader_per_registered_type_test) {
    auto assets = manager();

    const v3d::asset::Type registered[] = {
        v3d::asset::Type::ImagePng,
        v3d::asset::Type::ImageJpeg,
        v3d::asset::Type::ImageTga,
        v3d::asset::Type::JsonDocument,
        v3d::asset::Type::AudioWav,
        v3d::asset::Type::Text,
        v3d::asset::Type::Font2D,
        v3d::asset::Type::TextureFont
    };

    for (auto type : registered) {
        auto loader = assets->resolveLoader(type);
        BOOST_TEST(static_cast<bool>(loader));
        BOOST_TEST((loader->type() == type));
    }
}

BOOST_AUTO_TEST_CASE(manager_unregistered_type_test) {
    auto assets = manager();

    BOOST_CHECK_THROW(assets->resolveLoader(v3d::asset::Type::Undefined), std::invalid_argument);
}

/**
 * The extension picks the loader, and the asset that comes back is the one that loader
 * builds - which is what a dynamic_pointer_cast here is asking.
 **/
BOOST_AUTO_TEST_CASE(manager_type_from_extension_test) {
    auto assets = manager();

    auto document = assets->loadTypeFromExt("document.json");
    BOOST_TEST(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::Json>(document)));

    auto picture = assets->loadTypeFromExt("pixel.png");
    BOOST_TEST(static_cast<bool>(boost::dynamic_pointer_cast<v3d::asset::Image>(picture)));

    auto sound = assets->loadTypeFromExt("tone.wav");
    auto clip = boost::dynamic_pointer_cast<v3d::asset::Sound>(sound);
    BOOST_REQUIRE(clip);
    BOOST_TEST(static_cast<bool>(clip->clip()));
    BOOST_TEST(clip->clip()->audio() != nullptr);
}

/**
 * A wav that would not read comes back as no asset at all, the same as every other loader:
 * an asset holding no clip is indistinguishable from a loaded one until something plays it.
 **/
BOOST_AUTO_TEST_CASE(manager_unreadable_wav_test) {
    auto assets = manager();

    BOOST_TEST(!assets->load("nowhere.wav", v3d::asset::Type::AudioWav));
    BOOST_TEST(!assets->load("plain.txt", v3d::asset::Type::AudioWav));
}

/**
 * Text has a loader and a registered type but no extension, so the only way to a text asset
 * is load(name, Type::Text). Anything else at all is an exception rather than a null asset.
 **/
BOOST_AUTO_TEST_CASE(manager_unknown_extension_test) {
    auto assets = manager();

    BOOST_CHECK_THROW(assets->loadTypeFromExt("plain.txt"), std::invalid_argument);
    BOOST_CHECK_THROW(assets->loadTypeFromExt("document"), std::invalid_argument);
}

/**
 * A name is resolved against the manager's own path unless the caller says it is already
 * resolved, which is what a loader recursing into a second asset passes.
 **/
BOOST_AUTO_TEST_CASE(manager_path_resolution_test) {
    auto assets = manager("data");
    BOOST_TEST(static_cast<bool>(assets->load("document.json", v3d::asset::Type::JsonDocument)));

    auto rooted = manager("nowhere");
    BOOST_TEST(!rooted->load("document.json", v3d::asset::Type::JsonDocument));
    BOOST_TEST(static_cast<bool>(rooted->load("data/document.json", v3d::asset::Type::JsonDocument, true)));
}

/**
 * A file that is not there is a null asset, not a throw - Config::load reads its own return
 * that way and so does every consumer that guards the pointer.
 **/
BOOST_AUTO_TEST_CASE(manager_missing_asset_test) {
    auto assets = manager();

    BOOST_TEST(!assets->loadTypeFromExt("absent.json"));
    BOOST_TEST(!assets->loadTypeFromExt("absent.png"));
}
