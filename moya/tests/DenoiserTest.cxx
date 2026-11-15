/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <OpenImageDenoise/oidn.hpp>

namespace {

constexpr std::size_t kSize = 32;
constexpr float kGrey = 0.5F;
constexpr std::size_t kBorder = 4;

/** A noisy picture of a gradient, each float moved by a seeded draw, the same on every run. **/
std::vector<float> noisy(std::size_t size) {
    std::vector<float> out(size * size * 3);
    std::uint32_t seed = 7;
    for (std::size_t i = 0; i < out.size(); ++i) {
        seed = (seed * 1664525U) + 1013904223U;
        const float draw = static_cast<float>(seed >> 8U) / static_cast<float>(1U << 24U);
        out[i] = (static_cast<float>(i / 3 % size) / static_cast<float>(size)) + (0.5F * (draw - 0.5F));
    }
    return out;
}

/** A picture the RT filter denoises on a CPU device of a count of threads, or its own for zero. **/
// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the side, then the count of threads
std::vector<float> denoised(const std::vector<float> & colour, std::size_t size, int threads) {
    oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
    if (threads != 0) {
        device.set("numThreads", threads);
    }
    device.commit();
    std::vector<float> output(colour.size(), 0.0F);
    oidn::FilterRef filter = device.newFilter("RT");
    filter.setImage("color", const_cast<float *>(colour.data()), oidn::Format::Float3, size, size);
    filter.setImage("output", output.data(), oidn::Format::Float3, size, size);
    filter.set("hdr", true);
    filter.commit();
    filter.execute();
    return output;
}

// OIDN is built static with its CPU device alone, so a device of the default type is the CPU's
BOOST_AUTO_TEST_CASE(moya_denoiser_the_default_device_is_the_cpu_test) {
    oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::Default);
    device.commit();
    const char * message = nullptr;
    BOOST_TEST((device.getError(message) == oidn::Error::None));
    BOOST_TEST((device.get<oidn::DeviceType>("type") == oidn::DeviceType::CPU));
}

// a constant picture holds no noise, so the RT filter gives it back as it was. The filter reads
// each pixel's neighbours, and the outermost few pixels of a picture have fewer of them, so they
// are held only through the picture's mean
BOOST_AUTO_TEST_CASE(moya_denoiser_a_constant_picture_denoises_to_itself_test) {
    oidn::DeviceRef device = oidn::newDevice(oidn::DeviceType::CPU);
    device.commit();

    const std::size_t floats = kSize * kSize * 3;
    std::vector<float> colour(floats, kGrey);
    std::vector<float> albedo(floats, kGrey);
    std::vector<float> normal(floats, 0.0F);
    for (std::size_t pixel = 0; pixel < kSize * kSize; ++pixel) {
        normal[(pixel * 3) + 2] = 1.0F;
    }
    std::vector<float> output(floats, 0.0F);

    oidn::FilterRef filter = device.newFilter("RT");
    filter.setImage("color", colour.data(), oidn::Format::Float3, kSize, kSize);
    filter.setImage("albedo", albedo.data(), oidn::Format::Float3, kSize, kSize);
    filter.setImage("normal", normal.data(), oidn::Format::Float3, kSize, kSize);
    filter.setImage("output", output.data(), oidn::Format::Float3, kSize, kSize);
    filter.set("hdr", true);
    filter.commit();
    filter.execute();

    const char * message = nullptr;
    BOOST_TEST((device.getError(message) == oidn::Error::None));
    float furthest = 0.0F;
    double sum = 0.0;
    for (std::size_t y = 0; y < kSize; ++y) {
        for (std::size_t x = 0; x < kSize; ++x) {
            const float value = output[((y * kSize) + x) * 3];
            sum += value;
            if (x >= kBorder && y >= kBorder && x < kSize - kBorder && y < kSize - kBorder) {
                furthest = std::fmax(furthest, std::fabs(value - kGrey));
            }
        }
    }
    BOOST_TEST(furthest < 0.02F * kGrey);
    BOOST_TEST(std::fabs((sum / static_cast<double>(kSize * kSize)) - kGrey) < 0.005 * kGrey);
}

/**
 * The RT filter gives the same floats, bit for bit, on one thread, on every thread, and again on
 * every thread. A denoised picture is then the same on every run without a fixed count of
 * threads.
 *
 * Passes before the change: it checks OIDN itself, which the change relies on and does not alter.
 **/
BOOST_AUTO_TEST_CASE(moya_denoiser_the_filter_gives_one_picture_on_any_count_of_threads_test) {
    constexpr std::size_t kNoisy = 96;
    const std::vector<float> colour = noisy(kNoisy);
    const std::vector<float> one = denoised(colour, kNoisy, 1);
    const std::vector<float> every = denoised(colour, kNoisy, 0);
    const std::vector<float> again = denoised(colour, kNoisy, 0);
    BOOST_TEST(std::memcmp(one.data(), every.data(), one.size() * sizeof(float)) == 0);
    BOOST_TEST(std::memcmp(every.data(), again.data(), every.size() * sizeof(float)) == 0);
}

};  // namespace
