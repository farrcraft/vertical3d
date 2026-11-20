/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

/*
 * moya_bench times the new core's hierarchies over fixed scenes. For each scene it gives how long
 * it takes to gather and to build, and the memory its hierarchies take for each primitive. It
 * gives how many coherent and incoherent rays a second the traversal traces, at each instruction
 * set the processor runs. It prints that as one Markdown table. A second table times the
 * integrator over a lit room, and over the same room with a material of textures. It gives the
 * samples a second, the rays a second over the paths, and the shares of the time in traversal, in
 * materials' programs and in texture lookups. A third times the lit room with a compositor's
 * outputs. A fourth times the room filled with fog, the room with a statue that scatters below its
 * surface, and a lock of hair. It gives the shares of the time in tracking through media, in grid
 * lookups and in the hair model.
 */

#include <api/log/Logger.h>
#include <moya/bench/Measured.h>
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

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>
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
 * Times the integrator over a scene at an instruction set. The scene is rendered some times,
 * timed, and the fastest gives the samples a second. It is rendered once more, counted, for its
 * rays and the shares of its time. The counted render times every ray and every piece of work it
 * tallies, so its own time is not used. The rays a second are the counted render's rays over the
 * fastest timed render's time.
 **/
Measured measured(const scene::Scene & scene, cpu::Isa isa, std::uint32_t repeats, const boost::shared_ptr<log::Logger> & logger) {
    device::CpuDevice device(0, isa);
    integrate::Profile timed { .seconds = std::numeric_limits<double>::infinity() };
    film::Snapshot picture;
    for (std::uint32_t repeat = 0; repeat < repeats; ++repeat) {
        integrate::Profile once;
        picture = integrate::render(device, scene, logger, colour::Config(), {}, &once);
        timed.seconds = std::min(timed.seconds, once.seconds);
    }
    Measured out;
    out.counted.counted = true;
    integrate::render(device, scene, logger, colour::Config(), {}, &out.counted);
    std::uint64_t taken = 0;
    for (const std::uint32_t count : picture.samples) {
        taken += count;
    }
    const auto rays = static_cast<double>(out.counted.nearest + out.counted.any);
    out.samples = static_cast<double>(taken) / timed.seconds / kMillion;
    out.rays = rays / timed.seconds / kMillion;
    out.perSample = rays / static_cast<double>(taken);
    return out;
}

/**
 * Times the integrator over a lit scene at each instruction set, as measured() does, and prints a
 * row of the second table for each. It gives the shares of the counted render's time in
 * traversal, programs and texture lookups.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the side, then the samples per pixel
void lit(const std::string & name, const scene::Scene & room, const std::vector<cpu::Isa> & isas, std::uint32_t side, std::uint32_t samples,
    std::uint32_t repeats) {
    // what a render reports, such as a texture it cannot read, goes to the log
    const boost::shared_ptr<log::Logger> logger = boost::make_shared<log::Logger>();
    const std::size_t primitives = primitivesOf(geometry::gather(room, logger));
    for (const cpu::Isa isa : isas) {
        const Measured got = measured(room, isa, repeats, logger);
        std::cout << "| " << name << " | " << device::isaName(isa) << " | " << primitives << " | " << (side * side) << " | " << samples << " | "
                  << fixed(got.samples, 3) << " | " << fixed(got.rays, 3) << " | " << fixed(got.perSample, 2) << " | "
                  << fixed(100.0 * got.counted.traversal, 1) << " | " << fixed(100.0 * got.counted.programs, 1) << " | "
                  << fixed(100.0 * got.counted.lookups, 1) << " |\n";
    }
}

/**
 * Times the integrator over a scene of media, subsurface or hair at each instruction set, as
 * measured() does, and prints a row of the fourth table for each. It gives the shares of the
 * counted render's time in traversal, tracking through media, the volumes' grid lookups,
 * materials' programs and the hair model. Tracking's share holds the traversal, grid lookups and
 * programs it makes.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the side, then the samples per pixel
void media(const std::string & name, const scene::Scene & scene, const std::vector<cpu::Isa> & isas, std::uint32_t side, std::uint32_t samples,
    std::uint32_t repeats) {
    const boost::shared_ptr<log::Logger> logger = boost::make_shared<log::Logger>();
    const std::size_t primitives = primitivesOf(geometry::gather(scene, logger));
    for (const cpu::Isa isa : isas) {
        const Measured got = measured(scene, isa, repeats, logger);
        std::cout << "| " << name << " | " << device::isaName(isa) << " | " << primitives << " | " << (side * side) << " | " << samples << " | "
                  << fixed(got.samples, 3) << " | " << fixed(got.rays, 3) << " | " << fixed(got.perSample, 2) << " | "
                  << fixed(100.0 * got.counted.traversal, 1) << " | " << fixed(100.0 * got.counted.tracking, 1) << " | "
                  << fixed(100.0 * got.counted.grids, 1) << " | " << fixed(100.0 * got.counted.programs, 1) << " | "
                  << fixed(100.0 * got.counted.hair, 1) << " |\n";
    }
}

/**
 * Times the integrator over the lit room and over the room with its outputs at each instruction
 * set, the fastest of some renders of each, and prints a row of the third table for each. The row
 * gives the room with outputs' samples a second and that rate as a share of the plain room's. It
 * also gives the shares of its time the host took after each pass to add Cryptomatte coverage and
 * to join deep lists.
 **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the side, then the samples per pixel
void outputs(const scene::Scene & plain, const scene::Scene & covered, const std::vector<cpu::Isa> & isas, std::uint32_t side, std::uint32_t samples,
    std::uint32_t repeats) {
    const boost::shared_ptr<log::Logger> logger = boost::make_shared<log::Logger>();
    const double taken = static_cast<double>(side) * side * samples;
    for (const cpu::Isa isa : isas) {
        device::CpuDevice device(0, isa);
        integrate::Profile fastest { .seconds = std::numeric_limits<double>::infinity() };
        integrate::Profile fastestPlain { .seconds = std::numeric_limits<double>::infinity() };
        for (std::uint32_t repeat = 0; repeat < repeats; ++repeat) {
            integrate::Profile once;
            integrate::render(device, plain, logger, colour::Config(), {}, &once);
            fastestPlain = once.seconds < fastestPlain.seconds ? once : fastestPlain;
            integrate::render(device, covered, logger, colour::Config(), {}, &once);
            fastest = once.seconds < fastest.seconds ? once : fastest;
        }
        std::cout << "| lit room with outputs | " << device::isaName(isa) << " | " << fixed(taken / fastest.seconds / kMillion, 3) << " | "
                  << fixed(100.0 * fastestPlain.seconds / fastest.seconds, 1) << " | " << fixed(100.0 * fastest.coverage / fastest.seconds, 1) << " | "
                  << fixed(100.0 * fastest.deep / fastest.seconds, 1) << " |\n";
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
    const std::uint32_t pixels = quick ? kQuickPixels : kPixels;
    const std::uint32_t samples = quick ? kQuickSamples : kSamples;
    std::cout << "\n| lit scene | isa | primitives | pixels | samples per pixel | Msamples/s | Mrays/s | rays per sample | traversal % | program % | "
                 "texture % |\n";
    std::cout << "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    lit("lit room", *litRoom(scale, pixels, samples), isas, pixels, samples, repeats);
    // the textured room's image is written where the system keeps temporary files
    const std::string image = (boost::filesystem::temp_directory_path() / "moya_bench_image.exr").string();
    lit("textured room", *texturedRoom(scale, pixels, samples, image), isas, pixels, samples, repeats);
    std::cout << "\n| outputs | isa | Msamples/s | rate of the lit room % | coverage % | deep % |\n";
    std::cout << "|---|---|---:|---:|---:|---:|\n";
    outputs(*litRoom(scale, pixels, samples), *outputRoom(scale, pixels, samples), isas, pixels, samples, repeats);
    std::cout << "\n| media scene | isa | primitives | pixels | samples per pixel | Msamples/s | Mrays/s | rays per sample | traversal % | tracking % | "
                 "grid % | program % | hair % |\n";
    std::cout << "|---|---|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|---:|\n";
    // the fog room's grid is written where the system keeps temporary files
    const std::string grid = (boost::filesystem::temp_directory_path() / "moya_bench_fog.vdb").string();
    media("fog room", *fogRoom(scale, pixels, samples, grid), isas, pixels, samples, repeats);
    media("subsurface room", *subsurfaceRoom(scale, pixels, samples), isas, pixels, samples, repeats);
    media("hair lock", *hairLock(scale, pixels, samples), isas, pixels, samples, repeats);
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
