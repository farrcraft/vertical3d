/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/CieObserver.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

/**
 * moya_rgbtable's fit of Jakob and Hanika's sigmoid polynomials, in double precision.
 **/
namespace v3d::moya::tools::rgbtable {

using Vector = std::array<double, 3>;
using Matrix = std::array<Vector, 3>;

/**
 * Fits the three coefficients of a sigmoid polynomial whose spectrum, as a reflectance under ACES
 * white, has a given colour. The polynomial is in the wavelength normalised to [0, 1] over the
 * observer's range. A Gauss-Newton solve minimises the CIELAB difference between the colour
 * wanted and the colour the reflectance gives.
 *
 * After each step, coefficients whose largest exceeds kLargest are scaled down to it. A colour
 * with a channel at one lies on the edge of what a reflectance can reach, and the solve would
 * otherwise grow its coefficients without bound. Points so fitted differ from their neighbours
 * by orders of magnitude, and the table's interpolation between them gives a spectrum of neither
 * colour. Where such a solve stalls depends on rounding, so the table differs between compilers.
 **/
class Fit final {
 public:
    /** The solve's most iterations. **/
    static constexpr int kIterations = 15;
    /** The squared CIELAB difference the solve stops at. **/
    static constexpr double kConverged = 1e-6;
    /** The most the greatest coefficient may grow to, in the normalised wavelength. **/
    static constexpr double kLargest = 200.0;

    /**
     * Fit reads the observer's functions times ACES white, divided by the Y integral, at each
     * 1 nm wavelength: a reflectance's XYZ under ACES white is their sum weighted by it.
     **/
    Fit() : weights_(kernel::spectrum::kObserverRows) {
        for (int k = 0; k < kernel::spectrum::kObserverRows; ++k) {
            const double white = kernel::spectrum::acesWhite(static_cast<float>(kernel::spectrum::kObserverFirst + k));
            for (std::size_t j = 0; j < 3; ++j) {
                const double weight = kernel::spectrum::kObserver[k][j] * white / kernel::spectrum::kYIntegral;
                weights_[static_cast<std::size_t>(k)][j] = weight;
                white_[j] += weight;
            }
        }
    }

    /**
     * Fits the coefficients for a target XYZ, starting from those given and leaving the fit in
     * them. Returns the final squared CIELAB difference.
     **/
    double solve(const Vector & target, Vector * coefficients) const {
        const Vector wanted = lab(target);
        double squared = 0.0;
        for (int iteration = 0; iteration < kIterations; ++iteration) {
            const Vector miss = residual(wanted, *coefficients);
            squared = (miss[0] * miss[0]) + (miss[1] * miss[1]) + (miss[2] * miss[2]);
            if (squared < kConverged) {
                break;
            }
            Matrix jacobian {};
            for (std::size_t i = 0; i < 3; ++i) {
                Vector stepped = *coefficients;
                stepped[i] += kStep;
                const Vector moved = residual(wanted, stepped);
                for (std::size_t j = 0; j < 3; ++j) {
                    jacobian[j][i] = (moved[j] - miss[j]) / kStep;
                }
            }
            Vector step {};
            if (!cramer(jacobian, miss, &step)) {
                break;
            }
            // the residual's Jacobian is minus the colour's, so the Newton step subtracts
            for (std::size_t i = 0; i < 3; ++i) {
                (*coefficients)[i] -= step[i];
            }
            const double largest = std::max({ (*coefficients)[0], (*coefficients)[1], (*coefficients)[2] });
            if (largest > kLargest) {
                for (double & coefficient : *coefficients) {
                    coefficient *= kLargest / largest;
                }
            }
        }
        return squared;
    }

    /** Coefficients in the normalised wavelength as coefficients in nanometres. **/
    static Vector nanometres(const Vector & normalised) {
        const double first = kernel::spectrum::kObserverFirst;
        const double scale = 1.0 / kSpan;
        return { normalised[0] * scale * scale, (normalised[1] * scale) - (2.0 * normalised[0] * first * scale * scale),
            normalised[2] - (normalised[1] * first * scale) + (normalised[0] * first * first * scale * scale) };
    }

 private:
    /** The span of the observer's wavelengths, in its 1 nm steps. **/
    static constexpr double kSpan = kernel::spectrum::kObserverRows - 1;
    /** The step of the Jacobian's finite differences. **/
    static constexpr double kStep = 1e-5;
    /** A determinant smaller than this leaves the solve where it is. **/
    static constexpr double kSingular = 1e-15;
    /** CIELAB's knee: below its cube, the cube root becomes a line. **/
    static constexpr double kKnee = 6.0 / 29.0;
    /** The line's offset, 4 / 29, and the factor its slope divides by, 3. **/
    static constexpr double kLineOffset = 4.0 / 29.0;
    static constexpr double kLineSlope = 3.0;
    /** CIELAB's scales: lightness, its offset, red to green, and yellow to blue. **/
    static constexpr double kLightness = 116.0;
    static constexpr double kLightnessOffset = 16.0;
    static constexpr double kRedGreen = 500.0;
    static constexpr double kYellowBlue = 200.0;

    static double cieF(double ratio) {
        return ratio > kKnee * kKnee * kKnee ? std::cbrt(ratio) : (ratio / (kLineSlope * kKnee * kKnee)) + kLineOffset;
    }

    /** CIELAB from XYZ, against ACES white. **/
    Vector lab(const Vector & xyz) const {
        const double fx = cieF(xyz[0] / white_[0]);
        const double fy = cieF(xyz[1] / white_[1]);
        const double fz = cieF(xyz[2] / white_[2]);
        return { (kLightness * fy) - kLightnessOffset, kRedGreen * (fx - fy), kYellowBlue * (fy - fz) };
    }

    /** The XYZ of the reflectance some coefficients give, under ACES white. **/
    Vector reflected(const Vector & coefficients) const {
        Vector xyz { 0.0, 0.0, 0.0 };
        for (std::size_t k = 0; k < weights_.size(); ++k) {
            const double t = static_cast<double>(k) / kSpan;
            const double x = (((coefficients[0] * t) + coefficients[1]) * t) + coefficients[2];
            const double value = 0.5 + (x / (2.0 * std::sqrt(1.0 + (x * x))));
            for (std::size_t j = 0; j < 3; ++j) {
                xyz[j] += weights_[k][j] * value;
            }
        }
        return xyz;
    }

    /** The CIELAB colour wanted less the colour some coefficients give. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the colour wanted, then what gives one
    Vector residual(const Vector & wanted, const Vector & coefficients) const {
        const Vector have = lab(reflected(coefficients));
        return { wanted[0] - have[0], wanted[1] - have[1], wanted[2] - have[2] };
    }

    static double determinant(const Matrix & matrix) {
        return (matrix[0][0] * ((matrix[1][1] * matrix[2][2]) - (matrix[1][2] * matrix[2][1]))) -
            (matrix[0][1] * ((matrix[1][0] * matrix[2][2]) - (matrix[1][2] * matrix[2][0]))) +
            (matrix[0][2] * ((matrix[1][0] * matrix[2][1]) - (matrix[1][1] * matrix[2][0])));
    }

    /** Solves matrix * solution = rhs by Cramer's rule, or returns false for a singular matrix. **/
    static bool cramer(const Matrix & matrix, const Vector & rhs, Vector * solution) {
        const double whole = determinant(matrix);
        if (!(std::abs(whole) > kSingular)) {
            return false;
        }
        for (std::size_t column = 0; column < 3; ++column) {
            Matrix replaced = matrix;
            for (std::size_t row = 0; row < 3; ++row) {
                replaced[row][column] = rhs[row];
            }
            (*solution)[column] = determinant(replaced) / whole;
        }
        return true;
    }

    std::vector<Vector> weights_;
    Vector white_ { 0.0, 0.0, 0.0 };
};

};  // namespace v3d::moya::tools::rgbtable
