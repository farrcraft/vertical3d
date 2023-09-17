/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Wav.h"

#include <string>

#include "../Type.h"
#include "../Sound.h"

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

    /**
     **/
    Wav::Wav(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::AudioWav, logger) {
    }

    /**
     **/
    boost::shared_ptr<Asset> Wav::load(std::string_view name) {
        boost::shared_ptr<v3d::audio::AudioClip> clip = boost::make_shared<v3d::audio::AudioClip>();
        clip->load(name);
        boost::shared_ptr<Sound> asset = boost::make_shared<Sound>(std::string(name), Type::AudioWav, clip);

        return asset;
    }
};  // namespace v3d::asset::loader
