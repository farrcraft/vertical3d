/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <string>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>

#include "../Engine.h"
#include "Wav.h"

namespace {

boost::shared_ptr<v3d::audio::Engine> engine() {
    return boost::make_shared<v3d::audio::Engine>(
        boost::make_shared<v3d::log::Logger>(),
        boost::make_shared<entt::dispatcher>());
}

boost::shared_ptr<v3d::asset::Json> config(const std::string& text) {
    return boost::make_shared<v3d::asset::Json>(
        "sounds", v3d::asset::Type::JsonDocument, boost::json::parse(text).as_object());
}

/**
 * Stands in for the app's asset manager: records what it was asked for and reads the file
 * the way the Wav loader does, so a source that resolves to nothing comes back as no clip.
 **/
struct Resolver {
    boost::shared_ptr<v3d::audio::AudioClip> operator()(const std::string& source) {
        asked_.push_back(source);
        boost::shared_ptr<v3d::audio::AudioClip> clip =
            boost::make_shared<v3d::audio::AudioClip>();
        if (!clip->load(source)) {
            return boost::shared_ptr<v3d::audio::AudioClip>();
        }
        return clip;
    }

    std::vector<std::string> asked_;
};

};  // namespace

/**
 * The document is a list of clip ids over the sources that hold them. The library names a
 * source and the app resolves it - this library cannot reach the asset manager itself.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_load_test) {
    v3dtest::writeWav("hit.wav");
    v3dtest::writeWav("score.wav");

    Resolver resolve;
    auto sound = engine();
    BOOST_TEST(sound->load(config(R"({"sounds": [
        {"clip_id": "hit", "file": "hit.wav"},
        {"clip_id": "score", "file": "score.wav"}
    ]})"), std::ref(resolve)));

    // every source the document named reached the resolver, in the order it listed them
    BOOST_REQUIRE_EQUAL(resolve.asked_.size(), 2u);
    BOOST_CHECK_EQUAL(resolve.asked_[0], "hit.wav");
    BOOST_CHECK_EQUAL(resolve.asked_[1], "score.wav");
}

/**
 * A clip that will not resolve leaves the rest of the document loaded and reports itself, so
 * an app missing one wav still gets the sounds it has.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_load_missing_clip_test) {
    v3dtest::writeWav("hit.wav");

    Resolver resolve;
    auto sound = engine();
    BOOST_TEST(!sound->load(config(R"({"sounds": [
        {"clip_id": "hit", "file": "hit.wav"},
        {"clip_id": "gone", "file": "nowhere.wav"}
    ]})"), std::ref(resolve)));

    BOOST_CHECK_EQUAL(resolve.asked_.size(), 2u);
    // the clip that did not resolve was never filed, so nothing can ask for it
    BOOST_TEST(!sound->playClip("gone"));
}

/**
 * No resolver at all fails every clip, which is what an app that passes an empty one gets.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_load_without_resolver_test) {
    auto sound = engine();

    BOOST_TEST(!sound->load(config(R"({"sounds": [
        {"clip_id": "hit", "file": "hit.wav"}
    ]})"), v3d::audio::Engine::Resolve()));
    BOOST_TEST(!sound->playClip("hit"));
}

/**
 * Every rejection is a false return rather than an exception out of app startup - the lookups
 * are guarded because boost::json::object::at throws for a key it does not hold.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_no_sounds_key_test) {
    Resolver resolve;
    BOOST_TEST(!engine()->load(config(R"({"clips": []})"), std::ref(resolve)));
    BOOST_TEST(resolve.asked_.empty());
}

BOOST_AUTO_TEST_CASE(audio_engine_sounds_not_an_array_test) {
    Resolver resolve;
    BOOST_TEST(!engine()->load(config(R"({"sounds": {"clip_id": "hit"}})"), std::ref(resolve)));
}

BOOST_AUTO_TEST_CASE(audio_engine_entry_not_an_object_test) {
    Resolver resolve;
    BOOST_TEST(!engine()->load(config(R"({"sounds": ["hit.wav"]})"), std::ref(resolve)));
}

BOOST_AUTO_TEST_CASE(audio_engine_entry_missing_keys_test) {
    Resolver resolve;
    BOOST_TEST(!engine()->load(config(R"({"sounds": [{"clip_id": "hit"}]})"), std::ref(resolve)));
    BOOST_TEST(!engine()->load(config(R"({"sounds": [{"file": "hit.wav"}]})"), std::ref(resolve)));
    BOOST_TEST(resolve.asked_.empty());
}

/**
 * An empty list is a document that named no sounds rather than a malformed one.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_empty_sounds_test) {
    Resolver resolve;
    BOOST_TEST(engine()->load(config(R"({"sounds": []})"), std::ref(resolve)));
}

/**
 * playClip hands the clip's audio to the mixer, so a clip holding none is refused when it is
 * filed rather than reaching the mixer as a null.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_add_clip_test) {
    v3dtest::writeWav("hit.wav");

    auto sound = engine();
    boost::shared_ptr<v3d::audio::AudioClip> clip = boost::make_shared<v3d::audio::AudioClip>();
    BOOST_REQUIRE(clip->load("hit.wav"));
    BOOST_TEST(sound->addClip(clip, "hit"));

    BOOST_TEST(!sound->addClip(boost::make_shared<v3d::audio::AudioClip>(), "empty"));
    BOOST_TEST(!sound->addClip(boost::shared_ptr<v3d::audio::AudioClip>(), "null"));

    BOOST_TEST(!sound->playClip("empty"));
    BOOST_TEST(!sound->playClip("null"));
}

/**
 * A clip nothing filed is a false return, which is what the sound event handler logs. The
 * clip that did load is not played here: play needs the device that initialize() opens.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_unknown_clip_test) {
    BOOST_TEST(!engine()->playClip("nothing-under-this-name"));
}

/**
 * Shutdown reaches every clip it holds and the device it may never have opened, so an app
 * that fails before initialize() still tears down cleanly.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_shutdown_without_initialize_test) {
    v3dtest::writeWav("hit.wav");

    auto sound = engine();
    boost::shared_ptr<v3d::audio::AudioClip> clip = boost::make_shared<v3d::audio::AudioClip>();
    BOOST_REQUIRE(clip->load("hit.wav"));
    BOOST_TEST(sound->addClip(clip, "hit"));
    BOOST_CHECK_NO_THROW(sound->shutdown());
}
