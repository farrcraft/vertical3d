/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Wav.h"

#include <api/asset/Asset.h>
#include <api/asset/Loader.h>
#include <api/audio/AudioClip.h>
#include <api/audio/kind/Sound.h>
#include <api/log/Logger.h>

#include <string>
#include <string_view>

#include <boost/make_shared.hpp>

namespace v3d::audio::loader {

/**
 **/
Wav::Wav(const boost::shared_ptr<v3d::log::Logger>& logger) : v3d::asset::Loader(typeid(kind::Sound), logger) {
}

/**
 **/
boost::shared_ptr<v3d::asset::Asset> Wav::load(std::string_view name) {
    logger()->get()->info("Looking for wav asset at: {}", name);
    const boost::shared_ptr<AudioClip> clip = boost::make_shared<AudioClip>();
    // an asset holding no clip is indistinguishable from a loaded one until a consumer
    // dereferences it, so a read that failed comes back as no asset at all
    if (!clip->load(name)) {
        logger()->get()->error("Could not read wav asset: {}", name);
        return boost::shared_ptr<v3d::asset::Asset>();
    }
    boost::shared_ptr<kind::Sound> asset = boost::make_shared<kind::Sound>(std::string(name), clip);

    return asset;
}
};  // namespace v3d::audio::loader
