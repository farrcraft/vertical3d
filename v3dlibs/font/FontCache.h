/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "Font2D.h"

#include <map>
#include <string>

#include "../core/Logger.h"

#include <boost/shared_ptr.hpp>

namespace v3d::font {
    /**
     * A container of 2D fonts.
     */
    class FontCache {
     public:
        explicit FontCache(const boost::shared_ptr<v3d::core::Logger> & logger);
        ~FontCache();

        /**
         * Load a font
         * @param name a name used for referencing the font
         * @param face the typeface of the font
         * @param size the point size of the font
         */
        bool load(const std::string & name, const std::string & face, unsigned int size);
        /**
         * Get the named font.
         * @param name the name of the font to get
         * @return a pointer to the font or NULL if the font hasn't been loaded
         */
        boost::shared_ptr<Font2D> get(const std::string & name);

     private:
        std::map<std::string, boost::shared_ptr<Font2D> > fonts_;
        boost::shared_ptr<v3d::core::Logger> logger_;
    };

};  // namespace v3d::font
