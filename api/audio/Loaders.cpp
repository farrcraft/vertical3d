/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loaders.h"

#include <api/audio/loader/Wav.h>

#include <boost/make_shared.hpp>

namespace v3d::audio {

void registerLoaders(v3d::asset::Manager& manager, const boost::shared_ptr<v3d::log::Logger>& logger) {
    manager.registerLoader(boost::make_shared<loader::Wav>(logger), {".wav"});
}

};  // namespace v3d::audio
