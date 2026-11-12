/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

// moya_albedo integrates the albedos of GGX's single scattering, which energy compensation reads,
// and writes them as a C++ source that libmoya compiles. The build runs it.
//
// At each cosine and roughness of the grid, a reflection's albedo is the mean weight of
// reflections drawn through visible normals, on a stratified grid of the two random numbers. A
// reflection weighs the shadowing over the masking of outgoing, or nothing when it falls below the
// surface. The kernel's own GGX draws and weighs them, so the table and the lobe it compensates
// agree. Each roughness's mean albedo is twice the integral of the cosine times the directional
// albedo, read between the cosines as the kernel reads it.
//
// A dielectric's albedo is integrated the same way at each point of the ior axis too, from outside
// and from inside. Each visible normal both reflects, weighed by Fresnel's equations, and
// refracts, weighed by the rest, each by the shadowing over the masking when it leaves on its own
// side. The transmission is counted by the energy it carries. The shares a smooth interface
// transmits and a thin slab reflects are integrated over the cosine at each point of the axis.
//
// Usage: moya_albedo <output.cxx>

#include <moya/libmoya/kernel/bsdf/Albedos.h>
#include <moya/libmoya/kernel/bsdf/Fresnel.h>
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
#include <utility>
#include <vector>

#include <glm/geometric.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

namespace bsdf = v3d::moya::kernel::bsdf;

/** The points along the cosine, along the roughness and along the ior axis. **/
constexpr std::size_t kCosines = 32;
constexpr std::size_t kRoughnesses = 32;
constexpr std::size_t kIors = 16;

/** The side of the grid of random numbers a reflection's point draws, and a dielectric's. **/
constexpr std::size_t kSide = 128;
constexpr std::size_t kDielectricSide = 64;

/** The steps a share is integrated over the cosine in. **/
constexpr std::size_t kShareSteps = 20000;

/** The least cosine a point is integrated at, since a grazing view of none sees nothing. **/
constexpr float kLeastCosine = 1.0e-3F;

/** The sixths a span's integral is made of. **/
constexpr double kSixths = 6.0;

/** How many values the written table puts on each line. **/
constexpr std::size_t kPerLine = 8;

/** A table the source holds: the name of its array, and its values. **/
using Table = std::pair<const char *, std::vector<double>>;

/** The GGX distribution of a roughness. **/
bsdf::Ggx ggxAt(float roughness) {
    return bsdf::Ggx { glm::vec2 { std::max(roughness * roughness, bsdf::kLeast) } };
}

/** The direction at a cosine, in the plane of x and z. **/
glm::vec3 outgoingAt(float cosine) {
    const float z = std::max(cosine, kLeastCosine);
    return glm::vec3 { std::sqrt(1.0F - (z * z)), 0.0F, z };
}

/** The random numbers of a stratified grid's cell. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the row, the column, then the side
glm::vec2 cell(std::size_t row, std::size_t column, std::size_t side) {
    return glm::vec2 { (static_cast<float>(column) + 0.5F) / static_cast<float>(side), (static_cast<float>(row) + 0.5F) / static_cast<float>(side) };
}

/** A reflection's directional albedo at a cosine and a roughness, each from zero to one. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, then the roughness
double directional(float cosine, float roughness) {
    const bsdf::Ggx ggx = ggxAt(roughness);
    if (bsdf::Ggx::smooth(ggx.alpha)) {
        return 1.0;
    }
    const glm::vec3 outgoing = outgoingAt(cosine);
    const float masking = ggx.masking(outgoing);
    double sum = 0.0;
    for (std::size_t row = 0; row < kSide; ++row) {
        for (std::size_t column = 0; column < kSide; ++column) {
            const glm::vec3 normal = ggx.sampleVisible(outgoing, cell(row, column, kSide));
            const glm::vec3 incoming = (2.0F * glm::dot(outgoing, normal) * normal) - outgoing;
            if (incoming.z > 0.0F) {
                sum += static_cast<double>(ggx.shadowing(outgoing, incoming) / masking);
            }
        }
    }
    return sum / static_cast<double>(kSide * kSide);
}

/** A dielectric's directional albedo at a cosine, a roughness and a ratio of indices eta. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the cosine, the roughness, then eta
double dielectric(float cosine, float roughness, float eta) {
    const bsdf::Ggx ggx = ggxAt(roughness);
    if (bsdf::Ggx::smooth(ggx.alpha)) {
        return 1.0;
    }
    const glm::vec3 outgoing = outgoingAt(cosine);
    const float masking = ggx.masking(outgoing);
    double sum = 0.0;
    for (std::size_t row = 0; row < kDielectricSide; ++row) {
        for (std::size_t column = 0; column < kDielectricSide; ++column) {
            const glm::vec3 normal = ggx.sampleVisible(outgoing, cell(row, column, kDielectricSide));
            const float facing = glm::dot(outgoing, normal);
            const float reflected = bsdf::Fresnel::reflected(facing, eta);
            const glm::vec3 mirrored = (2.0F * facing * normal) - outgoing;
            if (mirrored.z > 0.0F) {
                sum += static_cast<double>(reflected * ggx.shadowing(outgoing, mirrored) / masking);
            }
            glm::vec3 bent { 0.0F };
            if (reflected < 1.0F && bsdf::Fresnel::refract(outgoing, normal, eta, bent) && bent.z < 0.0F) {
                sum += static_cast<double>((1.0F - reflected) * ggx.shadowing(outgoing, bent) / masking);
            }
        }
    }
    return sum / static_cast<double>(kDielectricSide * kDielectricSide);
}

/** The ratio of indices at a point of the ior axis. **/
float iorAt(std::size_t point) {
    const float axis = bsdf::kMostIorAxis * static_cast<float>(point) / static_cast<float>(kIors - 1);
    return (1.0F + axis) / (1.0F - axis);
}

/** Twice the integral of the cosine times a share at each cosine. **/
template <typename Share>
double overCosine(Share share) {
    double sum = 0.0;
    for (std::size_t i = 0; i < kShareSteps; ++i) {
        const float cosine = (static_cast<float>(i) + 0.5F) / static_cast<float>(kShareSteps);
        sum += 2.0 * static_cast<double>(share(cosine) * cosine) / static_cast<double>(kShareSteps);
    }
    return sum;
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

/** Every row's mean albedo, for a table of rows of cosines. **/
std::vector<double> averages(const std::vector<double> & table) {
    std::vector<double> out(table.size() / kCosines, 0.0);
    for (std::size_t row = 0; row < out.size(); ++row) {
        out[row] = average(table, row);
    }
    return out;
}

/**
 * A table of rows of cosines, a row for each roughness at each point of the ior axis, or for each
 * roughness alone when points is one. Each value is integrated by albedo, on oneTBB's threads.
 **/
template <typename Albedo>
std::vector<double> integrate(std::size_t points, Albedo albedo) {
    std::vector<double> table(points * kRoughnesses * kCosines, 0.0);
    tbb::parallel_for(tbb::blocked_range<std::size_t>(0, table.size()), [&](const tbb::blocked_range<std::size_t> & range) {
        for (std::size_t at = range.begin(); at != range.end(); ++at) {
            const std::size_t row = at / kCosines;
            const float cosine = static_cast<float>(at % kCosines) / (kCosines - 1);
            const float roughness = static_cast<float>(row % kRoughnesses) / (kRoughnesses - 1);
            table[at] = albedo(cosine, roughness, row / kRoughnesses);
        }
    });
    return table;
}

/**
 * Writes the tables as a C++ source that defines ggxAlbedos(). They are given in the order of the
 * view's fields: the reflection's two, then the dielectric's six.
 **/
bool write(const char * path, const std::vector<Table> & tables) {
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
    for (const Table & table : tables) {
        values(table.first, table.second);
    }
    out << "};  // namespace\n\n";
    out << "kernel::bsdf::Albedos ggxAlbedos() {\n";
    out << "    return { " << kCosines << "U, " << kRoughnesses << "U";
    for (std::size_t i = 0; i < tables.size(); ++i) {
        if (i == 2) {
            out << ", " << kIors << "U";
        }
        out << ", { " << tables[i].first << ", " << tables[i].second.size() << "U }";
    }
    out << " };\n}\n\n";
    out << "};  // namespace v3d::moya::material\n";
    return static_cast<bool>(out);
}

int run(const char * path) {
    const std::vector<double> reflection = integrate(1, [](float cosine, float roughness, std::size_t) { return directional(cosine, roughness); });
    const std::vector<double> entering =
        integrate(kIors, [](float cosine, float roughness, std::size_t point) { return dielectric(cosine, roughness, iorAt(point)); });
    const std::vector<double> leaving =
        integrate(kIors, [](float cosine, float roughness, std::size_t point) { return dielectric(cosine, roughness, 1.0F / iorAt(point)); });
    std::vector<double> transmitted;
    std::vector<double> slab;
    for (std::size_t point = 0; point < kIors; ++point) {
        const float eta = iorAt(point);
        transmitted.push_back(overCosine([eta](float cosine) { return 1.0F - bsdf::Fresnel::reflected(cosine, eta); }));
        slab.push_back(overCosine([eta](float cosine) { return bsdf::Fresnel::slab(cosine, eta); }));
    }
    const std::vector<double> average = averages(reflection);
    std::cout << "moya_albedo: " << reflection.size() << " points of reflection and " << (2 * entering.size())
              << " of a dielectric, the roughest mean albedo " << average.back() << "\n";
    const std::vector<Table> tables { { "kDirectional", reflection }, { "kAverage", average }, { "kEntering", entering }, { "kLeaving", leaving },
        { "kEnteringAverage", averages(entering) }, { "kLeavingAverage", averages(leaving) }, { "kTransmitted", transmitted }, { "kSlab", slab } };
    return write(path, tables) ? 0 : 1;
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
