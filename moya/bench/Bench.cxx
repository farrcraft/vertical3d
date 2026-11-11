/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

/*
 * moya_bench times the new core's hierarchies over fixed scenes. For each scene it gives how long
 * it takes to gather and to build, and the memory its hierarchies take for each primitive. It
 * gives how many coherent and incoherent rays a second the traversal traces, at each instruction
 * set the processor runs. It prints that as one Markdown table. A second table times the
 * integrator over a lit room. It gives the samples a second, the rays a second over the paths,
 * and the share of the time in traversal.
 */

#include <moya/bench/Rays.h>
#include <moya/bench/Scenes.h>
#include <moya/libmoya/accel/Assembly.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/device/cpu/Instance.h>
#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/cpu/MotionNode.h>
#include <moya/libmoya/device/cpu/Node.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/device/CpuLibrary.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/geometry/Geometry.h>
#include <moya/libmoya/geometry/Prototypes.h>
#include <moya/libmoya/integrate/Profile.h>
#include <moya/libmoya/integrate/Render.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/geometry/Key.h>
#include <moya/libmoya/kernel/geometry/Ray.h>
#include <moya/libmoya/scene/Scene.h>
#include <moya/libmoya/view/Traced.h>
#include <moya/libmoya/view/Tracing.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>

#include <boost/program_options.hpp>
#include <boost/shared_ptr.hpp>

#ifdef _MSC_VER
#include <intrin.h>
#elif defined(__x86_64__) || defined(__i386__)
#include <cpuid.h>
#endif

namespace v3d::moya::bench {

namespace {

namespace cpu = v3d::moya::device::cpu;

/** The seed the incoherent rays are drawn from. **/
constexpr std::uint32_t kRaySeed = 7;

/**
 * The rays to a side of the square of coherent rays, and how many times each list is traced, in
 * a full run and in a short pass. A short pass's scenes are smaller by the shrink.
 **/
constexpr std::uint32_t kSide = 1024;
constexpr std::uint32_t kRepeats = 3;
constexpr std::uint32_t kQuickSide = 32;
constexpr float kQuickShrink = 256.0F;

/** The lit room's pixels to a side and samples per pixel, in a full run and in a short pass. **/
constexpr std::uint32_t kPixels = 256;
constexpr std::uint32_t kSamples = 16;
constexpr std::uint32_t kQuickPixels = 16;
constexpr std::uint32_t kQuickSamples = 4;

/** A million, the rays a rate is given in, and the milliseconds in a second. **/
constexpr double kMillion = 1.0e6;
constexpr double kMilliseconds = 1000.0;

/**
 * The processor's leaves that name it: the one that gives the highest extended leaf, and the
 * first of the three that hold the name, sixteen characters each.
 **/
constexpr unsigned int kExtended = 0x80000000U;
constexpr unsigned int kBrand = 0x80000002U;
constexpr unsigned int kBrandLeaves = 3;

/** The processor's name, as it gives it, or "an unnamed processor" where it gives none. **/
std::string processor() {
    std::array<int, static_cast<std::size_t>(kBrandLeaves) * 4U> brand {};
#ifdef _MSC_VER
    std::array<int, 4> registers {};
    __cpuid(registers.data(), static_cast<int>(kExtended));
    if (static_cast<unsigned int>(registers[0]) >= kBrand + kBrandLeaves - 1U) {
        for (unsigned int leaf = 0; leaf < kBrandLeaves; ++leaf) {
            __cpuid(brand.data() + (static_cast<std::ptrdiff_t>(leaf) * 4), static_cast<int>(kBrand + leaf));
        }
    }
#elif defined(__x86_64__) || defined(__i386__)
    if (__get_cpuid_max(kExtended, nullptr) >= kBrand + kBrandLeaves - 1U) {
        for (unsigned int leaf = 0; leaf < kBrandLeaves; ++leaf) {
            unsigned int * at = reinterpret_cast<unsigned int *>(brand.data()) + (leaf * 4U);
            __get_cpuid(kBrand + leaf, at, at + 1, at + 2, at + 3);
        }
    }
#endif
    std::string out(reinterpret_cast<const char *>(brand.data()), strnlen(reinterpret_cast<const char *>(brand.data()), sizeof(brand)));
    out.erase(0, out.find_first_not_of(' '));
    out.erase(out.find_last_not_of(' ') + 1);
    return out.empty() ? std::string("an unnamed processor") : out;
}

/** The seconds a call takes. **/
double seconds(const std::function<void()> & call) {
    const auto start = std::chrono::steady_clock::now();
    call();
    return std::chrono::duration<double>(std::chrono::steady_clock::now() - start).count();
}

/** The bytes an assembly's hierarchies take: their nodes, references, instances and keys. **/
std::size_t bytesOf(const accel::Assembly & assembly) {
    return (assembly.nodes.size() * sizeof(cpu::Node)) + (assembly.motionNodes.size() * sizeof(cpu::MotionNode)) +
        (assembly.references.size() * sizeof(std::uint32_t)) + (assembly.instances.size() * sizeof(cpu::Instance)) +
        (assembly.keys.size() * sizeof(kernel::geometry::Key));
}

/** How many primitives the prototypes hold, each geometry's once however often it is placed. **/
std::size_t primitivesOf(const geometry::Prototypes & prototypes) {
    std::size_t out = 0;
    for (const geometry::Geometry & geometry : prototypes.geometries) {
        out += geometry.count;
    }
    return out;
}

/** The fastest of some traces of a list, in millions of rays a second, and the share that met. **/
std::pair<double, double> rate(device::CpuDevice & device, const view::Tracing & tracing, const std::vector<kernel::geometry::Ray> & rays,
    std::uint32_t repeats) {
    double fastest = std::numeric_limits<double>::infinity();
    std::size_t met = 0;
    for (std::uint32_t repeat = 0; repeat < repeats; ++repeat) {
        view::Traced traced;
        fastest = std::min(fastest, seconds([&]() {
            traced = view::trace(device, tracing, rays);
        }));
        met = static_cast<std::size_t>(std::ranges::count_if(traced.nearest, [](const kernel::geometry::Hit & hit) { return hit.found(); }));
    }
    return { static_cast<double>(rays.size()) / fastest / kMillion, 100.0 * static_cast<double>(met) / static_cast<double>(rays.size()) };
}

/** A number written to a count of decimal places. **/
std::string fixed(double value, int places) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(places) << value;
    return out.str();
}

/** The instruction sets to time: the one named, or every one the processor runs. **/
std::vector<cpu::Isa> isasOf(const std::string & named) {
    std::vector<cpu::Isa> out;
    for (const cpu::Isa isa : { cpu::Isa::Sse42, cpu::Isa::Avx2, cpu::Isa::Avx512 }) {
        if (isa <= device::newestIsa() && (named.empty() || named == device::isaName(isa))) {
            out.push_back(isa);
        }
    }
    if (out.empty()) {
        throw std::invalid_argument("this processor runs no instruction set named " + named);
    }
    return out;
}

/**
 * Times the integrator over the lit room at each instruction set, and prints a second table. The
 * room is rendered some times, timed, and the fastest gives the samples a second. It is rendered
 * once more, counted, for its rays and the share of its time in traversal. The counted render
 * times every ray, so its own time is not used. The rays a second are the counted render's rays
 * over the fastest timed render's time.
 **/
void lit(const std::vector<cpu::Isa> & isas, bool quick, std::uint32_t repeats) {
    const std::uint32_t side = quick ? kQuickPixels : kPixels;
    const std::uint32_t samples = quick ? kQuickSamples : kSamples;
    const boost::shared_ptr<scene::Scene> room = litRoom(quick ? 1.0F / kQuickShrink : 1.0F, side, samples);
    const std::size_t primitives = primitivesOf(geometry::gather(*room, {}));
    std::cout << "\n| lit scene | isa | primitives | pixels | samples per pixel | Msamples/s | Mrays/s | rays per sample | traversal % |\n";
    std::cout << "|---|---|---:|---:|---:|---:|---:|---:|---:|\n";
    for (const cpu::Isa isa : isas) {
        device::CpuDevice device(0, isa);
        integrate::Profile timed { .seconds = std::numeric_limits<double>::infinity() };
        film::Snapshot picture;
        for (std::uint32_t repeat = 0; repeat < repeats; ++repeat) {
            integrate::Profile once;
            picture = integrate::render(device, *room, {}, colour::Config(), {}, &once);
            timed.seconds = std::min(timed.seconds, once.seconds);
        }
        integrate::Profile counted { .counted = true };
        integrate::render(device, *room, {}, colour::Config(), {}, &counted);
        std::uint64_t taken = 0;
        for (const std::uint32_t count : picture.samples) {
            taken += count;
        }
        const auto rays = static_cast<double>(counted.nearest + counted.any);
        std::cout << "| lit room | " << device::isaName(isa) << " | " << primitives << " | " << (side * side) << " | " << samples << " | "
                  << fixed(static_cast<double>(taken) / timed.seconds / kMillion, 3) << " | " << fixed(rays / timed.seconds / kMillion, 3) << " | "
                  << fixed(rays / static_cast<double>(taken), 2) << " | " << fixed(100.0 * counted.traversal, 1) << " |\n";
    }
}

int run(int argc, char * argv[]) {
    boost::program_options::options_description described("Allowed options");
    described.add_options()("help", "produce help message")(
        "quick", "a short pass: the scenes 256 times smaller, 1024 rays of each kind, each traced once")(
        "isa", boost::program_options::value<std::string>(), "time one instruction set only, by the name the table gives it");
    boost::program_options::variables_map options;
    boost::program_options::store(boost::program_options::parse_command_line(argc, argv, described), options);
    boost::program_options::notify(options);
    if (options.count("help") != 0) {
        std::cout << described << "\n";
        return EXIT_SUCCESS;
    }
    // a short pass's scenes are 256 times smaller, and its 1024 rays of each kind are traced once
    const bool quick = options.count("quick") != 0;
    const float scale = quick ? 1.0F / kQuickShrink : 1.0F;
    const std::uint32_t side = quick ? kQuickSide : kSide;
    const std::uint32_t repeats = quick ? 1U : kRepeats;
    const std::vector<cpu::Isa> isas = isasOf(options.count("isa") != 0 ? options["isa"].as<std::string>() : std::string());

    std::cout << "moya_bench on " << processor() << ", " << std::thread::hardware_concurrency() << " threads, at a scale of "
              << scale << ", " << (side * side) << " rays of each kind, the fastest of " << repeats << ".\n\n";
    std::cout << "| scene | isa | primitives | gather ms | build ms | bytes per primitive | coherent Mrays/s | hit % | incoherent Mrays/s | hit % |\n";
    std::cout << "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    for (const Fixed & scene : scenes(scale)) {
        geometry::Prototypes prototypes;
        const double gathered = seconds([&]() {
            prototypes = geometry::gather(*scene.scene, {});
        });
        const std::vector<kernel::geometry::Ray> straight = coherent(scene.low, scene.high, side);
        const std::vector<kernel::geometry::Ray> scattered = incoherent(scene.low, scene.high, side * side, kRaySeed);
        for (const cpu::Isa isa : isas) {
            device::CpuDevice device(0, isa);
            accel::Assembly assembly;
            const double built = seconds([&]() {
                assembly = accel::assemble(device, prototypes);
            });
            const view::Tracing tracing(device, prototypes, assembly);
            const auto [straightRate, straightMet] = rate(device, tracing, straight, repeats);
            const auto [scatteredRate, scatteredMet] = rate(device, tracing, scattered, repeats);
            const std::size_t primitives = primitivesOf(prototypes);
            std::cout << "| " << scene.name << " | " << device::isaName(isa) << " | " << primitives << " | " << fixed(gathered * kMilliseconds, 1) << " | "
                      << fixed(built * kMilliseconds, 1) << " | " << fixed(static_cast<double>(bytesOf(assembly)) / static_cast<double>(primitives), 1) << " | "
                      << fixed(straightRate, 2) << " | " << fixed(straightMet, 1) << " | " << fixed(scatteredRate, 2) << " | " << fixed(scatteredMet, 1)
                      << " |\n";
        }
    }
    lit(isas, quick, repeats);
    return EXIT_SUCCESS;
}

};  // namespace

};  // namespace v3d::moya::bench

int main(int argc, char * argv[]) {
    // the option parser reports an error by throwing, and so does a failed allocation. An
    // exception leaving main aborts with no message, so each is written to stderr here
    try {
        return v3d::moya::bench::run(argc, argv);
    } catch (const std::exception & error) {
        std::fputs("error: ", stderr);
        std::fputs(error.what(), stderr);
        std::fputs("\n", stderr);
        return EXIT_FAILURE;
    } catch (...) {
        std::fputs("error: unrecognised failure\n", stderr);
        return EXIT_FAILURE;
    }
}
