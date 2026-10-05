/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <type_traits>

namespace v3d::type {

/**
 * A set of an enum's bits, typed by the enum.
 *
 * What an int mask was standing in for: it cannot be handed a bit of another enum or a stray
 * number, and asking whether a bit is set says which enum it is asking about. An enum opts in
 * with one operator|, which is what lets `A | B` spell a set:
 *
 *     constexpr type::Flags<Feature> operator|(Feature a, Feature b) { return type::Flags<Feature>(a) | b; }
 **/
template <typename E>
class Flags final {
    static_assert(std::is_enum_v<E>, "Flags holds the bits of an enum");
    using Bits = std::underlying_type_t<E>;

 public:
    constexpr Flags() noexcept = default;
    // implicit, so a single bit is a set of one wherever a set is asked for
    constexpr Flags(E bit) noexcept : bits_(static_cast<Bits>(bit)) {}  // NOLINT(runtime/explicit)

    constexpr Flags operator|(Flags other) const noexcept {
        return Flags(static_cast<Bits>(bits_ | other.bits_), 0);
    }

    constexpr Flags& operator|=(Flags other) noexcept {
        bits_ = static_cast<Bits>(bits_ | other.bits_);
        return *this;
    }

    /**
     * @return whether the bit is in the set
     **/
    constexpr bool has(E bit) const noexcept {
        return (bits_ & static_cast<Bits>(bit)) != 0;
    }

    constexpr bool empty() const noexcept {
        return bits_ == 0;
    }

    constexpr bool operator==(const Flags& other) const noexcept = default;

 private:
    constexpr Flags(Bits bits, int /* tag */) noexcept : bits_(bits) {}

    Bits bits_ = 0;
};

};  // namespace v3d::type
