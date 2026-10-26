/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/AudioClip.h>
#include <api/audio/Engine.h>
#include <api/audio/Events.h>
#include <api/audio/Listener.h>
#include <api/audio/Play.h>
#include <api/event/kind/Sound.h>
#include <api/log/Logger.h>

#include <SDL3/SDL_hints.h>

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <boost/json.hpp>
#include <boost/make_shared.hpp>

#include <glm/geometric.hpp>
#include <glm/vec3.hpp>

#include "Wav.h"

namespace {

boost::shared_ptr<v3d::audio::Engine> engine() {
    return boost::make_shared<v3d::audio::Engine>(
        boost::make_shared<v3d::log::Logger>(),
        boost::make_shared<entt::dispatcher>());
}

boost::json::object config(const std::string& text) {
    return boost::json::parse(text).as_object();
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

/**
 * The document lists a clip id and a source for each clip. The engine passes each source to
 * the resolver the app supplies, and stores what comes back under the clip id.
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
    BOOST_REQUIRE_EQUAL(resolve.asked_.size(), 2U);
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

    BOOST_CHECK_EQUAL(resolve.asked_.size(), 2U);
    // the clip that resolved was filed, and the one that did not was never filed
    BOOST_TEST(sound->has("hit"));
    BOOST_TEST(!sound->has("gone"));
}

/**
 * No resolver at all fails every clip, as it does for an app that passes an empty one.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_load_without_resolver_test) {
    auto sound = engine();

    BOOST_TEST(!sound->load(config(R"({"sounds": [
        {"clip_id": "hit", "file": "hit.wav"}
    ]})"), v3d::audio::Engine::Resolve()));
    BOOST_TEST(!sound->has("hit"));
}

/**
 * Every rejection is a false return, not an exception out of app startup. The lookups are
 * guarded because boost::json::object::at throws for a key it does not hold.
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
 * A clip_id or a file that is not a string is a rejection, not the exception
 * boost::json::value_to throws for it.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_entry_value_not_a_string_test) {
    Resolver resolve;
    BOOST_CHECK_NO_THROW(BOOST_TEST(!engine()->load(
        config(R"({"sounds": [{"clip_id": 3, "file": "hit.wav"}]})"), std::ref(resolve))));
    BOOST_CHECK_NO_THROW(BOOST_TEST(!engine()->load(
        config(R"({"sounds": [{"clip_id": "hit", "file": 3}]})"), std::ref(resolve))));
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
 * play() hands the clip's audio to the mixer. A clip holding none is refused when it is
 * stored, so the mixer never receives a null.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_add_clip_test) {
    v3dtest::writeWav("hit.wav");

    auto sound = engine();
    const boost::shared_ptr<v3d::audio::AudioClip> clip = boost::make_shared<v3d::audio::AudioClip>();
    BOOST_REQUIRE(clip->load("hit.wav"));
    BOOST_TEST(sound->addClip(clip, "hit"));

    BOOST_TEST(sound->has("hit"));

    BOOST_TEST(!sound->addClip(boost::make_shared<v3d::audio::AudioClip>(), "empty"));
    BOOST_TEST(!sound->addClip(boost::shared_ptr<v3d::audio::AudioClip>(), "null"));

    BOOST_TEST(!sound->has("empty"));
    BOOST_TEST(!sound->has("null"));
}

/**
 * A clip nothing filed is a false return, which the sound event handler logs. The
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
    const boost::shared_ptr<v3d::audio::AudioClip> clip = boost::make_shared<v3d::audio::AudioClip>();
    BOOST_REQUIRE(clip->load("hit.wav"));
    BOOST_TEST(sound->addClip(clip, "hit"));
    BOOST_CHECK_NO_THROW(sound->shutdown());
}

/**
 * The track functions with no device. The clip table and the voice bookkeeping can be
 * tested. Whether a sound is audible cannot, so these tests run without the device that
 * audio::Engine::initialize() opens.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_play_without_a_device_test) {
    v3dtest::writeWav("hit.wav");

    auto sound = engine();
    const boost::shared_ptr<v3d::audio::AudioClip> clip = boost::make_shared<v3d::audio::AudioClip>();
    BOOST_REQUIRE(clip->load("hit.wav"));
    BOOST_TEST(sound->addClip(clip, "hit"));

    // no device means no track to play on, and 0 is no voice at all
    v3d::audio::Play bed;
    bed.bus = "ambience";
    bed.loops = -1;
    bed.fadeInMs = 500;
    bed.gain = 0.4F;
    BOOST_CHECK(sound->play("hit", bed) == v3d::audio::Voice::None);
    BOOST_TEST(!sound->playClip("hit"));

    // and a voice nobody was given is not playing, cannot be stopped and takes no gain
    BOOST_TEST(!sound->playing(v3d::audio::Voice::None));
    BOOST_TEST(!sound->playing(v3d::audio::Voice{1}));
    BOOST_TEST(!sound->stop(v3d::audio::Voice{1}));
    BOOST_TEST(!sound->gain(v3d::audio::Voice{1}, 0.5F));

    BOOST_CHECK_EQUAL(sound->volume(v3d::audio::Voice{1}), 0.0F);
    BOOST_CHECK_NO_THROW(sound->stopAll(250));
}

/**
 * A bus level is stored whether or not a device is open, so a settings screen can apply a
 * volume read from its file before anything plays, and on a machine with no sound card.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_bus_level_is_kept_without_a_device_test) {
    auto sound = engine();

    // a bus never set is at full level, and so is no bus
    BOOST_CHECK_EQUAL(sound->busGain("music"), 1.0F);
    BOOST_CHECK_EQUAL(sound->busGain(""), 1.0F);

    BOOST_TEST(sound->busGain("music", 0.5F));
    BOOST_CHECK_EQUAL(sound->busGain("music"), 0.5F);
    // one bus's level leaves the others alone
    BOOST_CHECK_EQUAL(sound->busGain("sfx"), 1.0F);

    // the empty name is no bus, so it takes no level
    BOOST_TEST(!sound->busGain("", 0.5F));
    BOOST_CHECK_EQUAL(sound->busGain(""), 1.0F);

    // a level outlasts the device, so one set before initialize() is still there after it
    sound->initialize();
    sound->shutdown();
    BOOST_CHECK_EQUAL(sound->busGain("music"), 0.5F);
}

/**
 * Opens SDL's dummy audio driver, which plays into nothing and needs no sound card. The mixer
 * then gives out real tracks, so the volume each one is set to can be read back.
 **/
struct DummyDevice {
    DummyDevice() {
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        v3dtest::writeWav("bed.wav", 44100);
        clip = boost::make_shared<v3d::audio::AudioClip>();
        clip->load("bed.wav");
    }
    ~DummyDevice() {
        SDL_ResetHint(SDL_HINT_AUDIO_DRIVER);
    }
    DummyDevice(const DummyDevice&) = delete;
    DummyDevice& operator=(const DummyDevice&) = delete;

    /**
     * A bed that loops until stopped, so it is still playing when its volume is read.
     **/
    static v3d::audio::Play bed(const std::string& bus, float gain) {
        v3d::audio::Play how;
        how.bus = bus;
        how.loops = -1;
        how.gain = gain;
        return how;
    }

    boost::shared_ptr<v3d::audio::AudioClip> clip;
};

/**
 * A sound plays at its own gain times its bus's level: whether the level was set before it
 * started or while it played, and after its own gain changes.
 **/
BOOST_FIXTURE_TEST_CASE(audio_engine_a_sound_plays_at_its_gain_times_its_bus_test, DummyDevice) {
    auto sound = engine();
    BOOST_REQUIRE(sound->addClip(clip, "bed"));
    // set before the device opens, as an app applies the volumes it read from settings
    BOOST_TEST(sound->busGain("ambience", 0.5F));
    BOOST_REQUIRE(sound->initialize());

    const v3d::audio::Voice title = sound->play("bed", bed("ambience", 0.4F));
    BOOST_REQUIRE(title != v3d::audio::Voice::None);
    BOOST_CHECK_CLOSE(sound->volume(title), 0.2F, 0.01F);

    // a level set while the sound plays scales its own gain, rather than replacing it
    BOOST_TEST(sound->busGain("ambience", 1.0F));
    BOOST_CHECK_CLOSE(sound->volume(title), 0.4F, 0.01F);
    BOOST_TEST(sound->busGain("ambience", 0.25F));
    BOOST_CHECK_CLOSE(sound->volume(title), 0.1F, 0.01F);

    // a change of the sound's own gain keeps its bus's level
    BOOST_TEST(sound->gain(title, 0.8F));
    BOOST_CHECK_CLOSE(sound->volume(title), 0.2F, 0.01F);

    // and a sound started on the bus later takes the stored level too
    const v3d::audio::Voice world = sound->play("bed", bed("ambience", 0.8F));
    BOOST_REQUIRE(world != v3d::audio::Voice::None);
    BOOST_CHECK_CLOSE(sound->volume(world), 0.2F, 0.01F);

    // a bus never set, and no bus, leave a sound at its own gain
    const v3d::audio::Voice music = sound->play("bed", bed("music", 0.6F));
    const v3d::audio::Voice loose = sound->play("bed", bed("", 0.7F));
    BOOST_CHECK_CLOSE(sound->volume(music), 0.6F, 0.01F);
    BOOST_CHECK_CLOSE(sound->volume(loose), 0.7F, 0.01F);

    // and one bus's level does not reach a sound on another
    BOOST_TEST(sound->busGain("music", 0.5F));
    BOOST_CHECK_CLOSE(sound->volume(music), 0.3F, 0.01F);
    BOOST_CHECK_CLOSE(sound->volume(world), 0.2F, 0.01F);
    BOOST_CHECK_CLOSE(sound->volume(loose), 0.7F, 0.01F);

    sound->shutdown();
}

/**
 * The defaults are a one shot's: no bus, no repeat, no fade, and the clip's own volume.
 **/
BOOST_AUTO_TEST_CASE(audio_play_defaults_are_a_one_shot_test) {
    const v3d::audio::Play once;

    BOOST_CHECK_EQUAL(once.bus, "");
    BOOST_CHECK_EQUAL(once.loops, 0);
    BOOST_CHECK_EQUAL(once.fadeInMs, 0);
    BOOST_CHECK_CLOSE(once.gain, 1.0F, 0.001F);
}

/**
 * Shutdown after a play that found no device, and a second shutdown after the first, both
 * return without throwing. With no device, play() returns 0 and holds no track.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_shutdown_twice_after_a_play_without_a_device_test) {
    v3dtest::writeWav("hit.wav");

    auto sound = engine();
    const boost::shared_ptr<v3d::audio::AudioClip> clip = boost::make_shared<v3d::audio::AudioClip>();
    BOOST_REQUIRE(clip->load("hit.wav"));
    BOOST_TEST(sound->addClip(clip, "hit"));
    BOOST_CHECK(sound->play("hit", v3d::audio::Play()) == v3d::audio::Voice::None);

    BOOST_CHECK_NO_THROW(sound->shutdown());
    BOOST_CHECK_NO_THROW(sound->shutdown());
}

/**
 * The dispatcher is shared with the rest of the app and can outlive the engine. The delegate
 * initialize() hands it has to go when the engine does, whether or not anything called
 * shutdown(). That holds with or without a device, since initialize() connects whenever SDL's
 * audio subsystem starts, whether or not a device opens.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_lets_the_dispatcher_go_test) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    {
        v3d::audio::Engine sound(boost::make_shared<v3d::log::Logger>(), dispatcher);
        sound.initialize();
        BOOST_CHECK(!dispatcher->sink<v3d::event::kind::Sound>().empty());
    }
    BOOST_CHECK(dispatcher->sink<v3d::event::kind::Sound>().empty());

    // and shutdown() lets it go too, since a silent engine should not be asked to play
    v3d::audio::Engine sound(boost::make_shared<v3d::log::Logger>(), dispatcher);
    sound.initialize();
    sound.shutdown();
    BOOST_CHECK(dispatcher->sink<v3d::event::kind::Sound>().empty());
}

/**
 * Records the listener an event backend is handed, and nothing else.
 **/
class Recorder final : public v3d::audio::Events {
 public:
    bool bank(const std::string& /* path */) override { return false; }
    bool play(std::string_view /* event */) override { return false; }
    v3d::audio::Instance start(std::string_view /* event */) override { return v3d::audio::Instance::None; }
    bool play(std::string_view /* event */, const glm::vec3& /* position */) override { return false; }
    v3d::audio::Instance start(std::string_view /* event */, const glm::vec3& /* position */) override {
        return v3d::audio::Instance::None;
    }
    bool position(v3d::audio::Instance /* instance */, const glm::vec3& /* position */) override { return false; }
    std::optional<glm::vec3> position(v3d::audio::Instance /* instance */) const override { return std::nullopt; }
    bool listener(const v3d::audio::Listener& listener) override {
        heard.push_back(listener);
        return true;
    }
    bool stop(v3d::audio::Instance /* instance */, v3d::audio::Stop /* how */) override { return false; }
    bool playing(v3d::audio::Instance /* instance */) const override { return false; }
    bool parameter(std::string_view /* name */, float /* value */) override { return false; }
    bool volume(std::string_view /* path */, float /* level */) override { return false; }
    std::optional<float> volume(std::string_view /* path */) const override { return std::nullopt; }
    bool paused(std::string_view /* bus */, bool /* paused */) override { return false; }
    std::optional<bool> paused(std::string_view /* bus */) const override { return std::nullopt; }
    void update() override {}
    std::string_view name() const noexcept override { return "recorder"; }

    std::vector<v3d::audio::Listener> heard;
};

void checkVector(const std::optional<glm::vec3>& actual, const glm::vec3& expected) {
    BOOST_REQUIRE(actual.has_value());
    const glm::vec3 value = actual.value_or(glm::vec3(0.0F));
    BOOST_TEST_CONTEXT("expected (" << expected.x << ", " << expected.y << ", " << expected.z << ")") {
        BOOST_CHECK_SMALL(value.x - expected.x, 1.0e-5F);
        BOOST_CHECK_SMALL(value.y - expected.y, 1.0e-5F);
        BOOST_CHECK_SMALL(value.z - expected.z, 1.0e-5F);
    }
}

/**
 * A listener with +y up, built as glm::lookAt builds a view, with its right at forward x up.
 **/
v3d::audio::Listener facing(const glm::vec3& position, const glm::vec3& forward) {
    v3d::audio::Listener listener;
    listener.position = position;
    listener.forward = forward;
    listener.right = glm::cross(forward, listener.up);
    return listener;
}

/**
 * A placed sound is handed to the mixer relative to the listener, read back from the mixer
 * itself: where it starts, after the listener moves and turns, and after the sound moves.
 **/
BOOST_FIXTURE_TEST_CASE(audio_engine_a_placed_sound_is_heard_from_the_listener_test, DummyDevice) {
    auto sound = engine();
    BOOST_REQUIRE(sound->addClip(clip, "bed"));
    BOOST_REQUIRE(sound->initialize());

    v3d::audio::Play how = bed("", 1.0F);
    how.position = glm::vec3(3.0F, 0.0F, 0.0F);
    const v3d::audio::Voice voice = sound->play("bed", how);
    BOOST_REQUIRE(voice != v3d::audio::Voice::None);
    // the default listener is where the mixer's is
    checkVector(sound->heard(voice), {3.0F, 0.0F, 0.0F});

    // a listener that moves moves every placed sound around it
    BOOST_TEST(sound->listener(facing({3.0F, 0.0F, 5.0F}, {0.0F, 0.0F, -1.0F})));
    checkVector(sound->heard(voice), {0.0F, 0.0F, -5.0F});

    // and one that turns to face +x hears a sound on the x axis ahead of it
    BOOST_TEST(sound->listener(facing({0.0F, 0.0F, 0.0F}, {2.0F, 0.0F, 0.0F})));
    checkVector(sound->heard(voice), {0.0F, 0.0F, -3.0F});

    // a sound moved is heard from the listener as it is now
    BOOST_TEST(sound->position(voice, glm::vec3(0.0F, 2.0F, 4.0F)));
    checkVector(sound->heard(voice), {4.0F, 2.0F, 0.0F});

    // and a listener of the other hand hears the same point on its other side
    v3d::audio::Listener mirrored = facing({0.0F, 0.0F, 0.0F}, {1.0F, 0.0F, 0.0F});
    mirrored.right = -mirrored.right;
    BOOST_TEST(sound->listener(mirrored));
    checkVector(sound->heard(voice), {-4.0F, 2.0F, 0.0F});

    sound->shutdown();
}

/**
 * A sound is panned or placed, not both. A pan after a position leaves it unplaced, and a
 * position after a pan places it, as SDL_mixer itself holds a track in one mode or the other.
 **/
BOOST_FIXTURE_TEST_CASE(audio_engine_a_pan_and_a_position_clear_each_other_test, DummyDevice) {
    auto sound = engine();
    BOOST_REQUIRE(sound->addClip(clip, "bed"));
    BOOST_REQUIRE(sound->initialize());

    v3d::audio::Play how = bed("", 1.0F);
    how.pan = -0.5F;
    const v3d::audio::Voice voice = sound->play("bed", how);
    BOOST_REQUIRE(voice != v3d::audio::Voice::None);
    BOOST_TEST(!sound->heard(voice).has_value());

    BOOST_TEST(sound->position(voice, glm::vec3(1.0F, 0.0F, -1.0F)));
    checkVector(sound->heard(voice), {1.0F, 0.0F, -1.0F});

    BOOST_TEST(sound->pan(voice, 0.5F));
    BOOST_TEST(!sound->heard(voice).has_value());

    // a centred pan clears a position too, though it sets no gains of its own
    BOOST_TEST(sound->position(voice, glm::vec3(1.0F, 0.0F, -1.0F)));
    BOOST_TEST(sound->pan(voice, 0.0F));
    BOOST_TEST(!sound->heard(voice).has_value());

    // and a position given with a pan when the sound starts wins, so the sound is placed
    how.position = glm::vec3(0.0F, 0.0F, -2.0F);
    const v3d::audio::Voice placed = sound->play("bed", how);
    BOOST_REQUIRE(placed != v3d::audio::Voice::None);
    checkVector(sound->heard(placed), {0.0F, 0.0F, -2.0F});

    sound->shutdown();
}

/**
 * A sound event made with a position plays placed. The first voice an engine gives out is 1,
 * which is how the case finds the one shot the event started.
 **/
BOOST_FIXTURE_TEST_CASE(audio_engine_a_sound_event_with_a_position_plays_placed_test, DummyDevice) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    v3d::audio::Engine sound(boost::make_shared<v3d::log::Logger>(), dispatcher);
    BOOST_REQUIRE(sound.addClip(clip, "bed"));
    BOOST_REQUIRE(sound.initialize());
    BOOST_TEST(sound.listener(facing({0.0F, 0.0F, 2.0F}, {0.0F, 0.0F, -1.0F})));

    dispatcher->trigger(v3d::event::kind::Sound("bed", glm::vec3(0.0F, 1.0F, 0.0F)));
    checkVector(sound.heard(v3d::audio::Voice{1}), {0.0F, 1.0F, -2.0F});

    // and one made with a pan does not
    dispatcher->trigger(v3d::event::kind::Sound("bed", 0.5F));
    BOOST_TEST(sound.playing(v3d::audio::Voice{2}));
    BOOST_TEST(!sound.heard(v3d::audio::Voice{2}).has_value());

    sound.shutdown();
}

/**
 * The listener is kept with no device, made unit length and upright, and one that faces no way
 * is refused and leaves the last in place. A voice nobody was given takes no position.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_the_listener_is_kept_without_a_device_test) {
    auto sound = engine();
    BOOST_CHECK_EQUAL(sound->listener().forward.z, -1.0F);

    v3d::audio::Listener listener = facing({1.0F, 2.0F, 3.0F}, {4.0F, 0.0F, 0.0F});
    listener.up = glm::vec3(1.0F, 1.0F, 0.0F);
    listener.right = glm::vec3(0.0F, 0.0F, 2.0F);
    BOOST_TEST(sound->listener(listener));
    BOOST_CHECK_EQUAL(sound->listener().position.y, 2.0F);
    BOOST_CHECK_EQUAL(sound->listener().forward.x, 1.0F);
    BOOST_CHECK_SMALL(sound->listener().up.x, 1.0e-6F);
    BOOST_CHECK_CLOSE(sound->listener().up.y, 1.0F, 0.001F);

    BOOST_TEST(!sound->listener(facing({9.0F, 9.0F, 9.0F}, {0.0F, 3.0F, 0.0F})));
    BOOST_CHECK_EQUAL(sound->listener().position.x, 1.0F);
    BOOST_CHECK_EQUAL(sound->listener().forward.x, 1.0F);

    BOOST_TEST(!sound->position(v3d::audio::Voice{1}, glm::vec3(0.0F)));
    BOOST_TEST(!sound->heard(v3d::audio::Voice{1}).has_value());
}

/**
 * An attached event backend is given the listener at once and at every change after, as the
 * engine holds it, and is not given one the engine refused. Once detached it is given nothing.
 **/
BOOST_AUTO_TEST_CASE(audio_engine_an_attached_backend_hears_from_the_listener_test) {
    auto sound = engine();
    BOOST_TEST(sound->listener(facing({1.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -2.0F})));

    const boost::shared_ptr<Recorder> events = boost::make_shared<Recorder>();
    sound->attach(events);
    BOOST_REQUIRE_EQUAL(events->heard.size(), 1U);
    BOOST_CHECK_EQUAL(events->heard.back().position.x, 1.0F);
    BOOST_CHECK_EQUAL(events->heard.back().forward.z, -1.0F);

    BOOST_TEST(sound->listener(facing({0.0F, 5.0F, 0.0F}, {1.0F, 0.0F, 0.0F})));
    BOOST_REQUIRE_EQUAL(events->heard.size(), 2U);
    BOOST_CHECK_EQUAL(events->heard.back().position.y, 5.0F);
    BOOST_CHECK_EQUAL(events->heard.back().forward.x, 1.0F);

    BOOST_TEST(!sound->listener(facing({0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, 0.0F})));
    BOOST_CHECK_EQUAL(events->heard.size(), 2U);

    sound->attach(boost::shared_ptr<v3d::audio::Events>());
    BOOST_TEST(sound->listener(facing({0.0F, 0.0F, 0.0F}, {0.0F, 0.0F, -1.0F})));
    BOOST_CHECK_EQUAL(events->heard.size(), 2U);
}

};  // namespace
