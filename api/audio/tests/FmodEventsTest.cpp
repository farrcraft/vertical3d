/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/Events.h>
#include <api/log/Logger.h>

#include <SDL3/SDL_timer.h>
#include <spdlog/sinks/base_sink.h>

#include <algorithm>
#include <cstddef>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/test/unit_test.hpp>

namespace {

/**
 * Counts the warnings written to the process's log while it is attached, and detaches itself
 * when it goes out of scope. The log is global, so a sink left on it would count every case
 * after this one.
 **/
class Warnings final {
 public:
    explicit Warnings(const boost::shared_ptr<v3d::log::Logger>& logger) :
        logger_(logger->get()),
        sink_(std::make_shared<Sink>()) {
        logger_->sinks().push_back(sink_);
    }

    ~Warnings() {
        std::vector<spdlog::sink_ptr>& sinks = logger_->sinks();
        sinks.erase(std::remove(sinks.begin(), sinks.end(), sink_), sinks.end());
    }

    Warnings(const Warnings&) = delete;
    Warnings& operator=(const Warnings&) = delete;

    std::size_t count() const {
        return sink_->count;
    }

 private:
    class Sink final : public spdlog::sinks::base_sink<std::mutex> {
     public:
        std::size_t count = 0;

     protected:
        void sink_it_(const spdlog::details::log_msg& message) override {
            if (message.level == spdlog::level::warn) {
                ++count;
            }
        }
        void flush_() override {
        }
    };

    std::shared_ptr<spdlog::logger> logger_;
    std::shared_ptr<Sink> sink_;
};

BOOST_AUTO_TEST_SUITE(fmod_events_test)

/**
 * Built only with the FMOD backend, and run against the banks the SDK ships for its own examples.
 * events() hands back the FMOD backend. A real event plays once its banks are loaded, and an
 * event or a global parameter that no bank holds is refused rather than reported as played.
 **/
BOOST_AUTO_TEST_CASE(the_fmod_backend_plays_an_event_from_a_bank) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::shared_ptr<v3d::audio::Events> events = v3d::audio::events(logger);
    BOOST_REQUIRE(events->name() == "fmod");

    const std::string media = V3D_FMOD_EXAMPLE_BANKS;
    BOOST_TEST(!events->play("event:/UI/Cancel"));
    BOOST_REQUIRE(events->bank(media + "/Master.bank"));
    BOOST_REQUIRE(events->bank(media + "/Master.strings.bank"));
    BOOST_REQUIRE(events->bank(media + "/SFX.bank"));
    BOOST_TEST(events->play("event:/UI/Cancel"));
    events->update();

    BOOST_TEST(!events->bank(media + "/No Such.bank"));
    BOOST_TEST(!events->play("event:/No/Such Event"));
    BOOST_TEST(!events->parameter("No Such Parameter", 1.0F));
}

/**
 * A missing event and a refused parameter are each logged once per name, however often they are
 * asked for, and every call still returns false. The count is of lines written, because a
 * backend that warned on every call returned false just the same.
 **/
BOOST_AUTO_TEST_CASE(the_fmod_backend_reports_a_missing_name_once) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::shared_ptr<v3d::audio::Events> events = v3d::audio::events(logger);
    BOOST_REQUIRE(events->name() == "fmod");
    const Warnings warnings(logger);

    BOOST_TEST(!events->play("event:/No/Such Event"));
    BOOST_TEST(!events->play("event:/No/Such Event"));
    BOOST_TEST(warnings.count() == 1U);

    BOOST_TEST(!events->play("event:/No/Other Event"));
    BOOST_TEST(warnings.count() == 2U);

    for (int frame = 0; frame < 3; ++frame) {
        BOOST_TEST(!events->parameter("No Such Parameter", 1.0F));
    }
    BOOST_TEST(warnings.count() == 3U);

    // a parameter named as an event already reported is reported in its own right
    BOOST_TEST(!events->parameter("event:/No/Such Event", 1.0F));
    BOOST_TEST(warnings.count() == 4U);
}

/**
 * A looping event that start() started keeps playing across frames until it is stopped, and
 * stops when it is. The stop is the part a one shot cannot do, so the case checks the event is
 * playing first and stopped after, both read from FMOD's own playback state. A start() that
 * released its event as play() does would leave nothing to stop, and stop() would be refused.
 **/
BOOST_AUTO_TEST_CASE(the_fmod_backend_stops_an_event_it_started) {
    const boost::shared_ptr<v3d::log::Logger> logger = boost::make_shared<v3d::log::Logger>();
    const boost::shared_ptr<v3d::audio::Events> events = v3d::audio::events(logger);
    BOOST_REQUIRE(events->name() == "fmod");

    const std::string media = V3D_FMOD_EXAMPLE_BANKS;
    BOOST_REQUIRE(events->bank(media + "/Master.bank"));
    BOOST_REQUIRE(events->bank(media + "/Master.strings.bank"));
    BOOST_REQUIRE(events->bank(media + "/SFX.bank"));

    BOOST_CHECK(events->start("event:/No/Such Event") == v3d::audio::Instance::None);

    // FMOD carries out a start or a stop on its own thread, so a frame is an update() and a
    // short wait. A stopped event is seen within a few frames, and three seconds is ample
    const auto frame = [&events]() {
        SDL_Delay(10);
        events->update();
    };
    const auto stopped = [&events, &frame](v3d::audio::Instance instance) {
        for (int frames = 0; frames < 300 && events->playing(instance); ++frames) {
            frame();
        }
        return !events->playing(instance);
    };

    const v3d::audio::Instance bed = events->start("event:/Ambience/Country");
    BOOST_REQUIRE(bed != v3d::audio::Instance::None);
    // half a second of frames, longer than a one shot would keep an instance for
    for (int frames = 0; frames < 50; ++frames) {
        frame();
        BOOST_TEST(events->playing(bed));
    }

    BOOST_TEST(events->stop(bed, v3d::audio::Stop::Immediately));
    BOOST_TEST(stopped(bed));
    // released once stopped, so a second stop is refused
    BOOST_TEST(!events->stop(bed, v3d::audio::Stop::Immediately));

    // a second start is a new instance, and a fade out ends it too
    const v3d::audio::Instance again = events->start("event:/Ambience/Country");
    BOOST_REQUIRE(again != v3d::audio::Instance::None);
    BOOST_CHECK(again != bed);
    frame();
    BOOST_TEST(events->playing(again));
    BOOST_TEST(events->stop(again, v3d::audio::Stop::FadeOut));
    BOOST_TEST(stopped(again));
}

BOOST_AUTO_TEST_SUITE_END()

};  // namespace
