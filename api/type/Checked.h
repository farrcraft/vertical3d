/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>
#include <optional>

namespace v3d::type {

/**
 * The least and greatest values a conversion accepts, both inclusive.
 **/
template <typename T>
struct Range {
    T minimum{};
    T maximum{};
};

/**
 * Makes a float read from a file, the command line or a caller into an unsigned count.
 *
 * The value is refused when it is NaN, when it is infinite, and when it lies below minimum or
 * above maximum. The range is tested on the value as given, before its fraction is dropped, so
 * 2.5 is refused when the maximum is 2. A value that passes is truncated toward zero. A range with
 * minimum above maximum refuses every value.
 *
 * Converting a float outside an integer type's range is undefined behaviour. Every value this
 * returns came from a conversion that is defined.
 *
 * @return the count, or nothing when the value is refused
 **/
std::optional<uint32_t> toCount(float value, const Range<uint32_t>& range) noexcept;

/**
 * The same as toCount() with a minimum of 0.
 **/
std::optional<uint32_t> toCount(float value, uint32_t maximum) noexcept;

/**
 * Makes a float read from a file, the command line or a caller into a signed integer, such as
 * an index, a frame number or a handle.
 *
 * The rules are those of toCount: NaN, an infinity and a value outside [minimum, maximum] are
 * refused, the range is tested before the fraction is dropped, and a value that passes is
 * truncated toward zero. For example, -0.5 is refused when minimum is 0, and -2.5 becomes -2.
 *
 * @return the integer, or nothing when the value is refused
 **/
std::optional<int32_t> toInteger(float value, const Range<int32_t>& range) noexcept;

};  // namespace v3d::type
