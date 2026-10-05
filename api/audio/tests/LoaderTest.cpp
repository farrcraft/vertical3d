/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/asset/Manager.h>
#include <api/asset/Type.h>
#include <api/audio/Loaders.h>
#include <api/audio/kind/Sound.h>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

#include "Wav.h"

namespace {

boost::shared_ptr<v3d::asset::Manager> manager() {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    boost::shared_ptr<v3d::asset::Manager> assets = boost::make_shared<v3d::asset::Manager>("", logger);
    v3d::audio::registerLoaders(*assets, logger);
    return assets;
}

};  // namespace

/**
 * A wav reaches a manager only through this library, and comes back holding a clip the
 * mixer can play.
 **/
BOOST_AUTO_TEST_CASE(audio_loader_by_extension_test) {
    v3dtest::writeWav("loaded.wav");
    auto assets = manager();

    auto sound = boost::dynamic_pointer_cast<v3d::audio::kind::Sound>(assets->loadTypeFromExt("loaded.wav"));
    BOOST_REQUIRE(sound);
    BOOST_REQUIRE(sound->clip());
    BOOST_TEST(sound->clip()->audio() != nullptr);
}

/**
 * A wav that would not read comes back as no asset at all, the same as every other loader:
 * an asset holding no clip is indistinguishable from a loaded one until something plays it.
 **/
BOOST_AUTO_TEST_CASE(audio_loader_unreadable_test) {
    auto assets = manager();

    BOOST_TEST(!assets->load("nowhere.wav", v3d::asset::Type::AudioWav));
}
