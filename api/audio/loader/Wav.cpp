/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Wav.h"

#include <api/asset/Type.h>
#include <api/audio/kind/Sound.h>

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::audio::loader {

/**
 **/
Wav::Wav(const boost::shared_ptr<v3d::log::Logger>& logger) : v3d::asset::Loader(v3d::asset::Type::AudioWav, logger) {
}

/**
 **/
boost::shared_ptr<v3d::asset::Asset> Wav::load(std::string_view name) {
    logger_->get()->info("Looking for wav asset at: {}", name);
    boost::shared_ptr<AudioClip> clip = boost::make_shared<AudioClip>();
    // an asset holding no clip is indistinguishable from a loaded one until a consumer
    // dereferences it, so a read that failed comes back as no asset at all
    if (!clip->load(name)) {
        logger_->get()->error("Could not read wav asset: {}", name);
        return boost::shared_ptr<v3d::asset::Asset>();
    }
    boost::shared_ptr<kind::Sound> asset = boost::make_shared<kind::Sound>(std::string(name), v3d::asset::Type::AudioWav, clip);

    return asset;
}
};  // namespace v3d::audio::loader
