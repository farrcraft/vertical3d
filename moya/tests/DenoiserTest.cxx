/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <cmath>
#include <cstddef>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <OpenImageDenoise/oidn.hpp>

namespace {

constexpr std::size_t kSize = 32;
constexpr float kGrey = 0.5F;
constexpr std::size_t kBorder = 4;

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

};  // namespace
