/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include "Asset.h"
#include "../audio/AudioClip.h"

#include <boost/shared_ptr.hpp>

namespace v3d::asset {
    /**
     **/
    class Sound : public Asset {
    public:
        /**
         **/
        Sound(const std::string& name, Type t, boost::shared_ptr<v3d::audio::AudioClip> clip);

        /**
         **/
        boost::shared_ptr<v3d::audio::AudioClip> clip();

    private:
        boost::shared_ptr<v3d::audio::AudioClip> clip_;
    };
};  // namespace v3d::asset
