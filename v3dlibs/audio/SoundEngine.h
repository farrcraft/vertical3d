/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#pragma once

#include <map>
#include <string>

#include "AudioClip.h"

#include <boost/property_tree/ptree.hpp>

namespace v3d::audio {

    /**
     * The SoundEngine class. It is responsible for all of the OpenAL functionality. 
     **/
    class SoundEngine final {
     public:
        SoundEngine();
        ~SoundEngine() = default;

        void shutdown();
        bool load(const boost::property_tree::ptree & tree, const std::string & assetPath);
        bool loadClip(const std::string & filename, const std::string & key);
        bool playClip(const std::string & clip);
        void updateListener();

     private:
        std::map<std::string, AudioClip> sounds_;

        glm::vec3 listenerPosition_;
        glm::vec3 listenerVelocity_;
    };

};  // namespace v3d::audio
