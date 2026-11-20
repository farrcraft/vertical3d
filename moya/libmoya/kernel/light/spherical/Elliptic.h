/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/Qualifiers.h>

#include <cmath>
#include <cstdint>

namespace v3d::moya::kernel::light::spherical {

/*
 * Carlson's symmetric elliptic integrals, by duplication until the arguments agree within a
 * tolerance, then a series in what is left. Each tolerance is Numerical Recipes', which leaves an
 * error far below a float's. A loop stops after kDuplications whatever is left.
 */

/** The most duplications any of the integrals below takes. **/
inline constexpr std::uint32_t kDuplications = 32;

/** How far each duplication moves an argument towards the next: a quarter of the way. **/
inline constexpr float kShrink = 0.25F;

/** A third, which R_J's series and Legendre's third kind divide by. **/
inline constexpr float kThird = 1.0F / 3.0F;

/** The largest of three magnitudes. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - any order gives the same
MOYA_KERNEL inline float largest(float first, float second, float third) {
    const float pair = first > second ? first : second;
    return pair > third ? pair : third;
}

/** R_C(x, y), for x of zero or more and y above zero. **/
MOYA_KERNEL inline float carlsonRC(float x, float y) {
    constexpr float kTolerance = 0.0012F;
    constexpr float kC1 = 0.3F;
    constexpr float kC2 = 1.0F / 7.0F;
    constexpr float kC3 = 0.375F;
    constexpr float kC4 = 9.0F / 22.0F;
    float average = 0.0F;
    float spread = 0.0F;
    for (std::uint32_t step = 0; step < kDuplications; ++step) {
        const float lambda = (2.0F * std::sqrt(x) * std::sqrt(y)) + y;
        x = kShrink * (x + lambda);
        y = kShrink * (y + lambda);
        average = kThird * (x + y + y);
        spread = (y - average) / average;
        if (std::fabs(spread) <= kTolerance) {
            break;
        }
    }
    return (1.0F + (spread * spread * (kC1 + (spread * (kC2 + (spread * (kC3 + (spread * kC4)))))))) / std::sqrt(average);
}

/** R_F(x, y, z), for arguments of zero or more, at most one of them zero. **/
MOYA_KERNEL inline float carlsonRF(float x, float y, float z) {
    constexpr float kTolerance = 0.0025F;
    constexpr float kC1 = 1.0F / 24.0F;
    constexpr float kC2 = 0.1F;
    constexpr float kC3 = 3.0F / 44.0F;
    constexpr float kC4 = 1.0F / 14.0F;
    float average = 0.0F;
    float dx = 0.0F;
    float dy = 0.0F;
    float dz = 0.0F;
    for (std::uint32_t step = 0; step < kDuplications; ++step) {
        const float sx = std::sqrt(x);
        const float sy = std::sqrt(y);
        const float sz = std::sqrt(z);
        const float lambda = (sx * (sy + sz)) + (sy * sz);
        x = kShrink * (x + lambda);
        y = kShrink * (y + lambda);
        z = kShrink * (z + lambda);
        average = kThird * (x + y + z);
        dx = (average - x) / average;
        dy = (average - y) / average;
        dz = (average - z) / average;
        if (largest(std::fabs(dx), std::fabs(dy), std::fabs(dz)) <= kTolerance) {
            break;
        }
    }
    const float e2 = (dx * dy) - (dz * dz);
    const float e3 = dx * dy * dz;
    return (1.0F + (((kC1 * e2) - kC2 - (kC3 * e3)) * e2) + (kC4 * e3)) / std::sqrt(average);
}

/** R_J(x, y, z, w), for x, y and z of zero or more, at most one of them zero, and w above zero. **/
MOYA_KERNEL inline float carlsonRJ(float x, float y, float z, float w) {
    constexpr float kTolerance = 0.0015F;
    constexpr float kC1 = 3.0F / 14.0F;
    constexpr float kC2 = 1.0F / 3.0F;
    constexpr float kC3 = 3.0F / 22.0F;
    constexpr float kC4 = 3.0F / 26.0F;
    constexpr float kC5 = 0.75F * kC3;
    constexpr float kC6 = 1.5F * kC4;
    constexpr float kC7 = 0.5F * kC2;
    constexpr float kC8 = kC3 + kC3;
    constexpr float kFifth = 0.2F;
    float sum = 0.0F;
    float factor = 1.0F;
    float average = 0.0F;
    float dx = 0.0F;
    float dy = 0.0F;
    float dz = 0.0F;
    float dw = 0.0F;
    for (std::uint32_t step = 0; step < kDuplications; ++step) {
        const float sx = std::sqrt(x);
        const float sy = std::sqrt(y);
        const float sz = std::sqrt(z);
        const float lambda = (sx * (sy + sz)) + (sy * sz);
        const float alpha = (w * (sx + sy + sz)) + (sx * sy * sz);
        const float beta = w * (w + lambda) * (w + lambda);
        sum += factor * carlsonRC(alpha * alpha, beta);
        factor *= kShrink;
        x = kShrink * (x + lambda);
        y = kShrink * (y + lambda);
        z = kShrink * (z + lambda);
        w = kShrink * (w + lambda);
        average = kFifth * (x + y + z + w + w);
        dx = (average - x) / average;
        dy = (average - y) / average;
        dz = (average - z) / average;
        dw = (average - w) / average;
        if (largest(largest(std::fabs(dx), std::fabs(dy), std::fabs(dz)), std::fabs(dw), 0.0F) <= kTolerance) {
            break;
        }
    }
    const float ea = (dx * (dy + dz)) + (dy * dz);
    const float eb = dx * dy * dz;
    const float ec = dw * dw;
    const float ed = ea - (ec / kThird);
    const float ee = eb + (2.0F * dw * (ea - ec));
    const float series = 1.0F + (ed * (-kC1 + (kC5 * ed) - (kC6 * ee))) + (eb * (kC7 + (dw * (-kC8 + (dw * kC4))))) + (dw * ea * (kC2 - (dw * kC3))) -
        (kC2 * dw * ec);
    return (sum / kThird) + (factor * series / (average * std::sqrt(average)));
}

/**
 * Legendre's incomplete elliptic integral of the third kind: the integral from 0 to phi of
 * 1 / ((1 - n sin^2 t) sqrt(1 - m sin^2 t)). It takes phi in [0, pi/2], and the parameter m and
 * n each below one.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - n, phi and m, in Legendre's order
MOYA_KERNEL inline float ellipticPi(float n, float phi, float parameter) {
    const float s = std::sin(phi);
    const float cosine = std::cos(phi);
    const float squared = s * s;
    const float delta = 1.0F - (parameter * squared);
    return (s * carlsonRF(cosine * cosine, delta, 1.0F)) + ((n * kThird) * squared * s * carlsonRJ(cosine * cosine, delta, 1.0F, 1.0F - (n * squared)));
}

};  // namespace v3d::moya::kernel::light::spherical
