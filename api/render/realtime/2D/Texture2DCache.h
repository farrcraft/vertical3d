/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Texture2D.h"

#include <unordered_map>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
    /**
     **/
    class Texture2DCache {
     public:
        /**
         **/
        bool cache(const std::string& key, boost::shared_ptr<Texture2D> texture);
        
        /**
         **/
        boost::shared_ptr<Texture2D> fetch(const std::string& key);

     private:
        std::unordered_map<std::string, boost::shared_ptr<Texture2D>> textures_;
    };
};  // namespace v3d::render::realtime
