/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <api/asset/Asset.h>
#include <api/audio/AudioClip.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::audio::kind {
/**
 **/
class Sound : public v3d::asset::Asset {
 public:
    /**
     **/
    Sound(const std::string& name, v3d::asset::Type t, const boost::shared_ptr<AudioClip>& clip);

    /**
     **/
    boost::shared_ptr<AudioClip> clip();

 private:
    boost::shared_ptr<AudioClip> clip_;
};
};  // namespace v3d::audio::kind
