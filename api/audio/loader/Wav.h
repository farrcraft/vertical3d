/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Loader.h>

namespace v3d::audio::loader {
/**
 * Reads a WAV file into an AudioClip and gives a kind::Sound. A file that cannot be read gives
 * no asset.
 **/
class Wav final : public v3d::asset::Loader {
 public:
    /**
     **/
    explicit Wav(const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     **/
    boost::shared_ptr<v3d::asset::Asset> load(std::string_view name) override;
};
};  // namespace v3d::audio::loader
