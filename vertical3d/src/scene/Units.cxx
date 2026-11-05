/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Units.h"

#include <optional>
#include <string_view>

namespace v3d::editor::scene {

namespace {

/**
 * Each unit in metres. An inch is exactly 25.4 millimetres, and a foot exactly 304.8.
 **/
constexpr double MILLIMETRE = 0.001;
constexpr double CENTIMETRE = 0.01;
constexpr double KILOMETRE = 1000.0;
constexpr double INCH = 0.0254;
constexpr double FOOT = 0.3048;

};  // namespace

double metres(Length unit) noexcept {
    switch (unit) {
        case Length::Millimetre:
            return MILLIMETRE;
        case Length::Centimetre:
            return CENTIMETRE;
        case Length::Metre:
            return 1.0;
        case Length::Kilometre:
            return KILOMETRE;
        case Length::Inch:
            return INCH;
        case Length::Foot:
            return FOOT;
    }
    return 1.0;
}

std::string_view symbol(Length unit) noexcept {
    switch (unit) {
        case Length::Millimetre:
            return "mm";
        case Length::Centimetre:
            return "cm";
        case Length::Metre:
            return "m";
        case Length::Kilometre:
            return "km";
        case Length::Inch:
            return "in";
        case Length::Foot:
            return "ft";
    }
    return "m";
}

std::string_view name(Length unit) noexcept {
    switch (unit) {
        case Length::Millimetre:
            return "Millimetres";
        case Length::Centimetre:
            return "Centimetres";
        case Length::Metre:
            return "Metres";
        case Length::Kilometre:
            return "Kilometres";
        case Length::Inch:
            return "Inches";
        case Length::Foot:
            return "Feet";
    }
    return "Metres";
}

std::optional<Length> lengthOf(std::string_view symbol) noexcept {
    for (const Length unit : LENGTHS) {
        if (scene::symbol(unit) == symbol) {
            return unit;
        }
    }
    return std::nullopt;
}

};  // namespace v3d::editor::scene
