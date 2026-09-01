/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Loader.h"

namespace v3d::asset::loader {
    /**
     **/
    class Tga final : public Loader {
     public:
        /**
         **/
        Tga(Manager* manager, const boost::shared_ptr<v3d::log::Logger>& logger);

        /**
         **/
        boost::shared_ptr<Asset> load(std::string_view name);
    };
};  // namespace v3d::asset::loader
