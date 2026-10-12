/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/audio/Events.h>
#include <api/log/Logger.h>

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

};  // namespace

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
    BOOST_TEST(!events->parameter("No Such Parameter", 1.0f));
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
    BOOST_TEST(warnings.count() == 1u);

    BOOST_TEST(!events->play("event:/No/Other Event"));
    BOOST_TEST(warnings.count() == 2u);

    for (int frame = 0; frame < 3; ++frame) {
        BOOST_TEST(!events->parameter("No Such Parameter", 1.0f));
    }
    BOOST_TEST(warnings.count() == 3u);

    // a parameter named as an event already reported is reported in its own right
    BOOST_TEST(!events->parameter("event:/No/Such Event", 1.0f));
    BOOST_TEST(warnings.count() == 4u);
}

BOOST_AUTO_TEST_SUITE_END()
