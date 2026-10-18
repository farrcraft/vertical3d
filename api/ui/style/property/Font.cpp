/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Font.h"

#include <api/ui/style/Property.h>

#include <string>

namespace v3d::ui::style::property {

Font::Font(const Named& named) : Property(named.name), source_(named.source), italics_(false), bold_(false), size_(0) {
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

void Font::italics(bool on) {
    italics_ = on;
}

void Font::bold(bool on) {
    bold_ = on;
}

void Font::face(const std::string& name) {
    face_ = name;
}

void Font::size(unsigned int points) {
    size_ = points;
}

}  // namespace v3d::ui::style::property
