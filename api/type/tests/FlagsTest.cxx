/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include <api/type/Flags.h>

#include <cstdint>
#include <type_traits>
#include <utility>

#include <boost/test/unit_test.hpp>

using v3d::type::Flags;

namespace {

enum class Colour : uint8_t {
    Red = 1 << 0,
    Green = 1 << 1,
    Blue = 1 << 2
};

constexpr Flags<Colour> operator|(Colour a, Colour b) noexcept {
    return Flags<Colour>(a) | b;
}

/**
 * An enum that has not opted in, so two of its bits do not make a set.
 **/
enum class Shape : uint8_t {
    Square = 1 << 0,
    Circle = 1 << 1
};

template <typename E, typename = void>
struct Combines : std::false_type {};

template <typename E>
struct Combines<E, std::void_t<decltype(std::declval<E>() | std::declval<E>())>> : std::true_type {};

};  // namespace

BOOST_AUTO_TEST_SUITE(flags_test)

/**
 * A set made with no bits is empty and has none of them, and a single bit is a set of one.
 **/
BOOST_AUTO_TEST_CASE(flags_empty_and_one_bit_test) {
    constexpr Flags<Colour> none;
    BOOST_CHECK(none.empty());
    BOOST_CHECK(!none.has(Colour::Red));

    constexpr Flags<Colour> red = Colour::Red;
    BOOST_CHECK(!red.empty());
    BOOST_CHECK(red.has(Colour::Red));
    BOOST_CHECK(!red.has(Colour::Green));
    BOOST_CHECK(!red.has(Colour::Blue));
}

/**
 * An enum that opts in makes a set from `A | B`, and the set has those bits and no others.
 **/
BOOST_AUTO_TEST_CASE(flags_opt_in_or_test) {
    constexpr Flags<Colour> warm = Colour::Red | Colour::Green;
    static_assert(warm.has(Colour::Red) && warm.has(Colour::Green) && !warm.has(Colour::Blue));
    BOOST_CHECK(warm.has(Colour::Red));
    BOOST_CHECK(warm.has(Colour::Green));
    BOOST_CHECK(!warm.has(Colour::Blue));

    // a set and a bit combine too
    const Flags<Colour> all = warm | Colour::Blue;
    BOOST_CHECK(all.has(Colour::Blue));

    static_assert(Combines<Colour>::value, "an enum that opts in combines with |");
    static_assert(!Combines<Shape>::value, "an enum that has not opted in does not");
}

/**
 * |= adds bits to a set, and adding one it already has changes nothing.
 **/
BOOST_AUTO_TEST_CASE(flags_or_assign_test) {
    Flags<Colour> set;
    set |= Colour::Green;
    BOOST_CHECK(set.has(Colour::Green));
    BOOST_CHECK(!set.has(Colour::Red));

    set |= Colour::Red | Colour::Blue;
    BOOST_CHECK(set.has(Colour::Red));
    BOOST_CHECK(set.has(Colour::Blue));

    const Flags<Colour> before = set;
    set |= Colour::Green;
    BOOST_CHECK(set == before);
}

/**
 * Two sets are equal when they hold the same bits, whatever order they were added in.
 **/
BOOST_AUTO_TEST_CASE(flags_equality_test) {
    Flags<Colour> forwards = Colour::Red;
    forwards |= Colour::Blue;
    Flags<Colour> backwards = Colour::Blue;
    backwards |= Colour::Red;

    BOOST_CHECK(forwards == backwards);
    BOOST_CHECK(forwards == (Colour::Red | Colour::Blue));
    BOOST_CHECK(!(forwards == Flags<Colour>(Colour::Red)));
    BOOST_CHECK(Flags<Colour>() == Flags<Colour>());
    BOOST_CHECK(!(Flags<Colour>() == Flags<Colour>(Colour::Green)));
}

BOOST_AUTO_TEST_SUITE_END()
