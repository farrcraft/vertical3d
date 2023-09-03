/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Font.h"

namespace v3d::ui::style::prop {

Font::Font(const std::string& name, const std::string& src) : Property(name), source_(src), italics_(false), bold_(false) {
}

Font::~Font() {
}

bool Font::italics() const {
    return italics_;
}

bool Font::bold() const {
    return bold_;
}

std::string Font::face() const {
    return face_;
}

unsigned int Font::size() const {
    return size_;
}

std::string Font::source() const {
    return source_;
}

}  // namespace v3d::ui::style::prop
