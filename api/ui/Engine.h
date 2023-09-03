/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <vector>

#include "Container.h"
#include "style/Theme.h"

#include "../asset/Json.h"
#include "../log/Logger.h"

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::ui {

    class Engine {
     public:
        explicit Engine(const boost::shared_ptr<v3d::log::Logger>& logger);

        bool load(const boost::shared_ptr<v3d::asset::Json>& config);

     protected:
        bool loadMenu(const boost::json::object& component);

     private:
        boost::shared_ptr<v3d::log::Logger> logger_;
        std::vector<boost::shared_ptr<Container>> containers_;
        std::vector<boost::shared_ptr<style::Theme>> themes_;
    };

};  // namespace v3d::ui
