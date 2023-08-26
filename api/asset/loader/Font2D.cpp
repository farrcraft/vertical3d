/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Font2D.h"
#include "../Font2D.h"
#include "../Type.h"
#include "../../font/Font2D.h"

#include <boost/make_shared.hpp>

namespace v3d::asset::loader {

    /**
     **/
    Font2D::Font2D(Manager& manager,const boost::shared_ptr<v3d::log::Logger>& logger) : Loader(manager, Type::FONT_2D, logger) {
    }

    /**
     **/
    boost::shared_ptr<Asset> Font2D::load(std::string_view name) {
        boost::optional<ParameterValue> param = parameter("fontSize");
        if (!param) {
            return nullptr;
        }
        unsigned int fontSize = std::get<unsigned int>(param.get());
        boost::shared_ptr<v3d::font::Font2D> font = boost::make_shared<v3d::font::Font2D>(std::string(name), fontSize);
        font->build(logger_);

        boost::shared_ptr<Asset> asset = boost::make_shared<v3d::asset::Font2D>(std::string(name), type(), font);
        return asset;
    }

};  // namespace v3d::asset::loader
