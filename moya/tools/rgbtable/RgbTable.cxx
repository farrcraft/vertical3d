/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// moya_rgbtable fits Jakob and Hanika's RGB-to-spectrum table for ACEScg, and writes it as a C++
// source that libmoya compiles. The build runs it.
//
// The table holds three cubes, one for each channel that can be a colour's largest. At each point
// of a cube, Fit finds the coefficients whose reflectance under ACES white has the point's
// ACEScg colour. Each solve starts from the last solution along the largest channel's axis, from
// a fifth of the way up outwards, so that it starts close.
//
// Usage: moya_rgbtable <output.cxx>

#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/tools/rgbtable/Fit.h>
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

#include <glm/vec3.hpp>

namespace {

namespace rgbtable = v3d::moya::tools::rgbtable;
namespace spectrum = v3d::moya::kernel::spectrum;

/** The points along each axis of a cube. **/
constexpr std::size_t kResolution = 64;

/** The points of the three cubes. **/
constexpr std::size_t kPoints = 3 * kResolution * kResolution * kResolution;

/** A CIELAB difference above this is a point the fit could not reach. **/
constexpr float kMissed = 1.0F;

/** How many values the written table puts on each line: nodes, then coefficients. **/
constexpr std::size_t kNodesPerLine = 8;
constexpr std::size_t kCoefficientsPerLine = 6;

/** The largest channel's value at a point on its axis: a double smoothstep, dense at each end. **/
double node(std::size_t point) {
    // NOLINTNEXTLINE(readability-magic-numbers) - smoothstep's own polynomial
    const auto smoothstep = [](double t) { return t * t * (3.0 - (2.0 * t)); };
    return smoothstep(smoothstep(static_cast<double>(point) / static_cast<double>(kResolution - 1)));
}

/** A point's first coefficient: by cube, then by the largest channel, the third and the second. **/
std::size_t index(std::size_t largest, std::size_t zi, std::size_t yi, std::size_t xi) {
    return ((((((largest * kResolution) + zi) * kResolution) + yi) * kResolution) + xi) * 3;
}

/**
 * Fits one row of a cube, along the largest channel's axis, into the table, and records each
 * point's final CIELAB difference.
 **/
void fitRow(const rgbtable::Fit & fit, std::size_t largest, std::size_t xi, std::size_t yi, rgbtable::Table * table) {
    const double x = static_cast<double>(xi) / static_cast<double>(kResolution - 1);
    const double y = static_cast<double>(yi) / static_cast<double>(kResolution - 1);
    const auto store = [&](std::size_t zi, rgbtable::Vector * coefficients) {
        const double z = node(zi);
        glm::vec3 rgb(0.0F);
        rgb[static_cast<int>(largest)] = static_cast<float>(z);
        rgb[static_cast<int>((largest + 1) % 3)] = static_cast<float>(x * z);
        rgb[static_cast<int>((largest + 2) % 3)] = static_cast<float>(y * z);
        const glm::vec3 xyz = spectrum::acescgToXyz(rgb);
        const double squared = fit.solve({ xyz.x, xyz.y, xyz.z }, coefficients);
        const rgbtable::Vector out = rgbtable::Fit::nanometres(*coefficients);
        const std::size_t at = index(largest, zi, yi, xi);
        for (std::size_t k = 0; k < 3; ++k) {
            table->coefficients[at + k] = static_cast<float>(out[k]);
        }
        table->misses[at / 3] = static_cast<float>(std::sqrt(squared));
    };
    const std::size_t start = kResolution / 5;
    rgbtable::Vector coefficients { 0.0, 0.0, 0.0 };
    for (std::size_t zi = start; zi < kResolution; ++zi) {
        store(zi, &coefficients);
    }
    coefficients = { 0.0, 0.0, 0.0 };
    for (std::size_t zi = start + 1; zi-- > 0;) {
        store(zi, &coefficients);
    }
}

/** Writes the table as a C++ source that defines acescgTable(). **/
bool write(const char * path, const std::vector<float> & table) {
    std::ofstream out(path, std::ios::binary);
    out << "// Fitted by moya_rgbtable when moya was built. Do not edit.\n\n";
    out << "#include <moya/libmoya/colour/RgbTables.h>\n\n";
    out << "namespace v3d::moya::colour {\n\nnamespace {\n\n";
    std::array<char, 32> buffer {};
    const auto value = [&buffer](double v) {
        std::snprintf(buffer.data(), buffer.size(), "%.8eF,", v);
        return buffer.data();
    };
    out << "const float kNodes[" << kResolution << "] = {\n";
    for (std::size_t zi = 0; zi < kResolution; ++zi) {
        out << (zi % kNodesPerLine == 0 ? "    " : " ") << value(node(zi)) << (zi % kNodesPerLine == kNodesPerLine - 1 ? "\n" : "");
    }
    out << "};\n\nconst float kCoefficients[" << table.size() << "] = {\n";
    for (std::size_t i = 0; i < table.size(); ++i) {
        out << (i % kCoefficientsPerLine == 0 ? "    " : " ") << value(table[i]) << (i % kCoefficientsPerLine == kCoefficientsPerLine - 1 ? "\n" : "");
    }
    out << "};\n\n};  // namespace\n\n";
    out << "kernel::spectrum::RgbTable acescgTable() {\n";
    out << "    return { " << kResolution << "U, { kNodes, " << kResolution << "U }, { kCoefficients, " << table.size() << "U } };\n}\n\n";
    out << "};  // namespace v3d::moya::colour\n";
    return static_cast<bool>(out);
}

int run(const char * path) {
    const rgbtable::Fit fit;
    rgbtable::Table table { std::vector<float>(kPoints * 3), std::vector<float>(kPoints, 0.0F) };
    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, 3 * kResolution * kResolution), [&](const tbb::blocked_range<std::size_t> & range) {
        for (std::size_t task = range.begin(); task != range.end(); ++task) {
            const std::size_t largest = task / (kResolution * kResolution);
            fitRow(fit, largest, task % kResolution, (task / kResolution) % kResolution, &table);
        }
    });
    const auto missed = std::count_if(table.misses.begin(), table.misses.end(), [](float miss) { return miss > kMissed; });
    std::cout << "moya_rgbtable: " << kPoints << " points, " << missed << " beyond a CIELAB difference of " << kMissed << ", the largest "
              << *std::max_element(table.misses.begin(), table.misses.end()) << "\n";
    return write(path, table.coefficients) ? 0 : 1;
}

};  // namespace

int main(int argc, char ** argv) {
    if (argc != 2) {
        std::fputs("usage: moya_rgbtable <output.cxx>\n", stderr);
        return 2;
    }
    // the C streams report an error without throwing one of their own
    try {
        return run(argv[1]);
    } catch (const std::exception & error) {
        std::fprintf(stderr, "moya_rgbtable: %s\n", error.what());
    } catch (...) {
        std::fputs("moya_rgbtable: an unknown error\n", stderr);
    }
    return 1;
}
