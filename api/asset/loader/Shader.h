/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Loader.h"

namespace v3d::asset::loader {
    /**
     **/
    class Shader final : public Loader {
     public:
        /**
         **/
        Shader(Manager* manager, Type t, const boost::shared_ptr<v3d::log::Logger>& logger);

        /**
         **/
        boost::shared_ptr<Asset> load(std::string_view name);
    };
};  // namespace v3d::asset::loader
