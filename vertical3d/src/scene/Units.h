/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace v3d::editor::scene {

/**
 * The unit of length a project measures in: what one unit of the scene is.
 *
 * Changing it changes what the numbers mean, not the numbers, so nothing in the scene moves. A
 * file read in metres is scaled into it as it is imported. A new project measures in metres.
 **/
enum class Length : std::uint8_t { Millimetre, Centimetre, Metre, Kilometre, Inch, Foot };

/**
 * Every unit of length, in the order a list offers them.
 **/
constexpr std::array<Length, 6> LENGTHS = { Length::Millimetre, Length::Centimetre, Length::Metre, Length::Kilometre, Length::Inch, Length::Foot };

/**
 * @return how many metres one of a unit is
 **/
double metres(Length unit) noexcept;

/**
 * @return a unit's symbol, which a length is shown with and a project stores, such as "m"
 **/
std::string_view symbol(Length unit) noexcept;

/**
 * @return a unit's name, such as "Metres", for a list to offer
 **/
std::string_view name(Length unit) noexcept;

/**
 * @return the unit a symbol stands for, or nothing for one that stands for none
 **/
std::optional<Length> lengthOf(std::string_view symbol) noexcept;

};  // namespace v3d::editor::scene
