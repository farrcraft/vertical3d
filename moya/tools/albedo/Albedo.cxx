/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// moya_albedo integrates the albedo of a GGX reflection with a Fresnel factor of one, which
// energy compensation reads, and writes it as a C++ source that libmoya compiles. The build runs
// it.
//
// At each cosine and roughness of the grid, the albedo is the mean weight of reflections drawn
// through visible normals, on a stratified grid of the two random numbers. A reflection weighs the
// shadowing over the masking of outgoing, or nothing when it falls below the surface. The
// kernel's own GGX draws and weighs them, so the table and the lobe it compensates agree. Each
// roughness's mean albedo is twice the integral of the cosine times the directional albedo, read
// between the cosines as the kernel reads it.
//
// Usage: moya_albedo <output.cxx>

#include <moya/libmoya/kernel/bsdf/Ggx.h>
#include <tbb/blocked_range.h>
#include <tbb/parallel_for.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <cstdio>
#include <exception>
#include <fstream>
#include <iostream>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

namespace bsdf = v3d::moya::kernel::bsdf;

/** The points along the cosine and along the roughness. **/
constexpr std::size_t kCosines = 32;
constexpr std::size_t kRoughnesses = 32;

/** The side of the grid of random numbers each point draws. **/
constexpr std::size_t kSide = 128;

/** The least cosine a point is integrated at, since a grazing view of none sees nothing. **/
constexpr float kLeastCosine = 1.0e-3F;

/** The sixths a span's integral is made of. **/
constexpr double kSixths = 6.0;

/** How many values the written table puts on each line. **/
constexpr std::size_t kPerLine = 8;

/** The directional albedo at a cosine and a roughness, each from zero to one. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the roughness
double directional(float cosine, float roughness) {
    const float alpha = std::max(roughness * roughness, bsdf::kLeast);
    const bsdf::Ggx ggx { glm::vec2 { alpha } };
    if (bsdf::Ggx::smooth(ggx.alpha)) {
        return 1.0;
    }
    const float z = std::max(cosine, kLeastCosine);
    const glm::vec3 outgoing { std::sqrt(1.0F - (z * z)), 0.0F, z };
    const float masking = ggx.masking(outgoing);
    double sum = 0.0;
    for (std::size_t row = 0; row < kSide; ++row) {
        for (std::size_t column = 0; column < kSide; ++column) {
            const glm::vec2 u { (static_cast<float>(column) + 0.5F) / kSide, (static_cast<float>(row) + 0.5F) / kSide };
            const glm::vec3 normal = ggx.sampleVisible(outgoing, u);
            const glm::vec3 incoming = (2.0F * glm::dot(outgoing, normal) * normal) - outgoing;
            if (incoming.z > 0.0F) {
                sum += static_cast<double>(ggx.shadowing(outgoing, incoming) / masking);
            }
        }
    }
    return sum / static_cast<double>(kSide * kSide);
}

/**
 * Twice the integral of the cosine times a row's albedo, read linearly between its cosines. On
 * a span from a to b, the integral is (b - a) / 6 times the sum of two terms: the low end's albedo
 * times (2a + b), and the high end's times (a + 2b).
 **/
double average(const std::vector<double> & table, std::size_t row) {
    double sum = 0.0;
    for (std::size_t i = 0; i + 1 < kCosines; ++i) {
        const double low = static_cast<double>(i) / (kCosines - 1);
        const double high = static_cast<double>(i + 1) / (kCosines - 1);
        const double first = table[(row * kCosines) + i];
        const double second = table[(row * kCosines) + i + 1];
        sum += (high - low) / kSixths * ((first * ((2.0 * low) + high)) + (second * (low + (2.0 * high))));
    }
    return 2.0 * sum;
}

/** Writes the table as a C++ source that defines ggxAlbedos(). **/
bool write(const char * path, const std::vector<double> & table, const std::vector<double> & averages) {
    std::ofstream out(path, std::ios::binary);
    out << "// Integrated by moya_albedo when moya was built. Do not edit.\n\n";
    out << "#include <moya/libmoya/material/Albedos.h>\n\n";
    out << "namespace v3d::moya::material {\n\nnamespace {\n\n";
    std::array<char, 32> buffer {};
    const auto value = [&buffer](double v) {
        std::snprintf(buffer.data(), buffer.size(), "%.8eF,", v);
        return buffer.data();
    };
    const auto values = [&](const char * name, const std::vector<double> & held) {
        out << "const float " << name << "[" << held.size() << "] = {\n";
        for (std::size_t i = 0; i < held.size(); ++i) {
            out << (i % kPerLine == 0 ? "    " : " ") << value(held[i]) << (i % kPerLine == kPerLine - 1 || i + 1 == held.size() ? "\n" : "");
        }
        out << "};\n\n";
    };
    values("kDirectional", table);
    values("kAverage", averages);
    out << "};  // namespace\n\n";
    out << "kernel::bsdf::Albedos ggxAlbedos() {\n";
    out << "    return { " << kCosines << "U, " << kRoughnesses << "U, { kDirectional, " << table.size() << "U }, { kAverage, " << averages.size()
        << "U } };\n}\n\n";
    out << "};  // namespace v3d::moya::material\n";
    return static_cast<bool>(out);
}

int run(const char * path) {
    std::vector<double> table(kCosines * kRoughnesses, 0.0);
    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, table.size()), [&](const tbb::blocked_range<std::size_t> & range) {
        for (std::size_t at = range.begin(); at != range.end(); ++at) {
            const std::size_t row = at / kCosines;
            const float cosine = static_cast<float>(at % kCosines) / (kCosines - 1);
            const float roughness = static_cast<float>(row) / (kRoughnesses - 1);
            table[at] = directional(cosine, roughness);
        }
    });
    std::vector<double> averages(kRoughnesses, 0.0);
    for (std::size_t row = 0; row < kRoughnesses; ++row) {
        averages[row] = average(table, row);
    }
    std::cout << "moya_albedo: " << table.size() << " points, the roughest mean albedo " << averages.back() << "\n";
    return write(path, table, averages) ? 0 : 1;
}

};  // namespace

int main(int argc, char ** argv) {
    if (argc != 2) {
        std::fputs("usage: moya_albedo <output.cxx>\n", stderr);
        return 2;
    }
    // the C streams report an error without throwing one of their own
    try {
        return run(argv[1]);
    } catch (const std::exception & error) {
        std::fprintf(stderr, "moya_albedo: %s\n", error.what());
    } catch (...) {
        std::fputs("moya_albedo: an unknown error\n", stderr);
    }
    return 1;
}
