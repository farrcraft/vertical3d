/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Events.h"

#include <stdexcept>

#include "NullEvents.h"
#ifdef V3D_AUDIO_FMOD
#include "FmodEvents.h"
#endif

#include <boost/make_shared.hpp>

namespace v3d::audio {

/**
 **/
boost::shared_ptr<Events> events(const boost::shared_ptr<v3d::log::Logger>& logger) {
#ifdef V3D_AUDIO_FMOD
    try {
        boost::shared_ptr<Events> fmod = boost::make_shared<FmodEvents>(logger);
        logger->get()->info("Playing audio events through FMOD Studio");
        return fmod;
    } catch (const std::runtime_error& error) {
        logger->get()->error("{} - audio events will not play", error.what());
    }
#else
    logger->get()->info("No audio event backend was built, so audio events will not play");
#endif
    return boost::make_shared<NullEvents>();
}

};  // namespace v3d::audio
