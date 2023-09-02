/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Property.h"

#include <string>

namespace v3d::ui::style::prop
{

    /**
     * A style property that defines a font.
     */
    class Font : public Property
    {
    public:
        Font(const std::string& name, const std::string& src);
        ~Font();

        bool italics() const;
        bool bold() const;
        std::string face() const;
        unsigned int size() const;
        std::string source() const;

    private:
        std::string source_;
        bool italics_;
        bool bold_;
        std::string face_;
        unsigned int size_;
    };

}; // end namespace v3d::ui::style::prop
