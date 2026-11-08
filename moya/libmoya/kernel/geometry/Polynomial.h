/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cstdint>

namespace v3d::moya::kernel::geometry {

/** The highest degree rootsIn() solves. **/
inline constexpr std::uint32_t kHighestDegree = 4;

/** How many times rootsIn() halves an interval that holds a root. **/
inline constexpr std::uint32_t kHalvings = 40;

/** A polynomial's value at x, its coefficients rising from the constant. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the degree, then where it is read
MOYA_KERNEL inline float evaluate(const float * coefficients, std::uint32_t degree, float x) {
    float out = coefficients[degree];
    for (std::uint32_t power = degree; power > 0; --power) {
        out = (out * x) + coefficients[power - 1U];
    }
    return out;
}

/**
 * The root of a polynomial in [lower, upper], over which it is monotone and changes sign or is
 * zero at an end, by halving the interval.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the degree, then the interval
MOYA_KERNEL inline float rootBetween(const float * coefficients, std::uint32_t degree, float lower, float upper) {
    float atLower = evaluate(coefficients, degree, lower);
    for (std::uint32_t halving = 0; halving < kHalvings; ++halving) {
        const float middle = (lower + upper) * 0.5F;
        if (middle <= lower || middle >= upper) {
            break;
        }
        const float atMiddle = evaluate(coefficients, degree, middle);
        if ((atMiddle < 0.0F) == (atLower < 0.0F) && atMiddle != 0.0F) {
            lower = middle;
            atLower = atMiddle;
        } else {
            upper = middle;
        }
    }
    return (lower + upper) * 0.5F;
}

/**
 * The roots of a polynomial in [lower, upper], in rising order, given the turning points between
 * which it is monotone, rising too. Gives how many there are.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the degree, the turns, then the interval
MOYA_KERNEL inline std::uint32_t rootsBetween(const float * polynomial, std::uint32_t degree, const float * turns, std::uint32_t count,
    float lower, float upper, float * out) {
    std::uint32_t roots = 0;
    const auto add = [&](float root) {
        if (roots == 0 || out[roots - 1U] != root) {
            out[roots++] = root;
        }
    };
    float from = lower;
    for (std::uint32_t edge = 0; edge <= count; ++edge) {
        const float to = edge < count ? turns[edge] : upper;
        const float atFrom = evaluate(polynomial, degree, from);
        const float atTo = evaluate(polynomial, degree, to);
        if (atFrom == 0.0F) {
            add(from);
        } else if (atTo == 0.0F) {
            add(to);
        } else if ((atFrom < 0.0F) != (atTo < 0.0F)) {
            add(rootBetween(polynomial, degree, from, to));
        }
        from = to;
    }
    return roots;
}

/**
 * The real roots in [lower, upper] of a polynomial of degree at most four, its coefficients
 * rising from the constant, in rising order. Gives how many there are.
 *
 * Each root is isolated between the turning points of the polynomial, which are the roots of its
 * derivative, found the same way from the derivative's own. Between two turning points the
 * polynomial is monotone, so it holds a root there exactly when its sign changes, and halving
 * the interval finds it to the precision of a float. A double root, where the polynomial touches
 * zero without crossing, is found only where it rounds to zero.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the degree, then the interval
MOYA_KERNEL inline std::uint32_t rootsIn(const float * coefficients, std::uint32_t degree, float lower, float upper, float * out) {
    // derivatives[k] holds the k-th derivative, of degree degree - k
    float derivatives[kHighestDegree + 1U][kHighestDegree + 1U] {};
    for (std::uint32_t power = 0; power <= degree; ++power) {
        derivatives[0][power] = coefficients[power];
    }
    for (std::uint32_t order = 1; order < degree; ++order) {
        for (std::uint32_t power = 0; power + order <= degree; ++power) {
            derivatives[order][power] = derivatives[order - 1U][power + 1U] * static_cast<float>(power + 1U);
        }
    }
    // the roots of each derivative, from the linear one up to the polynomial itself, are the
    // turning points of the one above it
    float turns[kHighestDegree + 2U] {};
    std::uint32_t count = 0;
    for (std::uint32_t order = degree; order > 0; --order) {
        float found[kHighestDegree + 2U] {};
        count = rootsBetween(derivatives[order - 1U], degree - (order - 1U), turns, count, lower, upper, found);
        for (std::uint32_t root = 0; root < count; ++root) {
            turns[root] = found[root];
        }
    }
    for (std::uint32_t root = 0; root < count; ++root) {
        out[root] = turns[root];
    }
    return count;
}

};  // namespace v3d::moya::kernel::geometry
