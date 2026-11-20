/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Channel.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/film/Combine.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/output/Checkpoint.h>

#include <array>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/test/unit_test.hpp>

namespace {

namespace kernel = v3d::moya::kernel;
namespace film = v3d::moya::film;
typedef v3d::moya::device::CpuDevice CpuDevice;
using kernel::film::Combine;

/** The samples combined() adds to each pixel: each one's weight, depth and value. **/
constexpr int kSamples = 4;
constexpr float kWeights[kSamples] = { 1.0F, 2.0F, 0.5F, 1.5F };
constexpr float kDepths[kSamples] = { 3.0F, 1.0F, 2.0F, 1.0F };
constexpr float kValues[kSamples] = { 4.0F, -1.0F, 6.0F, 2.0F };

/** The channels: one of each combine, of two components but for the mean's one. **/
constexpr std::size_t kChannels = 5;
constexpr const char * kNames[kChannels] = { "mean", "sum", "closest", "least", "most" };
constexpr Combine kCombines[kChannels] = { Combine::Mean, Combine::Sum, Combine::Closest, Combine::Least, Combine::Most };

/** Where combined() writes, and the value its samples add to the first component. **/
struct Params {
    kernel::film::Pixels pixels;
    std::uint32_t offsets[5] { 0, 0, 0, 0, 0 };
    float offset { 0.0F };
};

/** The film's channels, as kNames and kCombines give them. **/
std::vector<film::Channel> channels() {
    std::vector<film::Channel> out;
    out.reserve(kChannels);
    for (std::size_t i = 0; i < kChannels; ++i) {
        out.push_back({ .name = kNames[i], .components = i == 0 ? 1U : 2U, .combine = kCombines[i] });
    }
    return out;
}

/**
 * A sample's value: the second component is the first negated, so a least and a most differ in
 * which sample each component keeps.
 **/
std::array<float, 2> valueOf(const Params & params, int sample) {
    const float first = kValues[sample] + params.offset;
    return { first, -first };
}

/** Adds kSamples samples to each pixel but those of the second column, which take none. **/
void combined(const Params & params, kernel::Item item) {
    if (item.x == 1U) {
        return;
    }
    for (int sample = 0; sample < kSamples; ++sample) {
        params.pixels.add(item, kWeights[sample], kernel::spectrum::SampledSpectrum(0.0F), kernel::spectrum::SampledWavelengths::visible(0.5F));
        const std::array<float, 2> value = valueOf(params, sample);
        params.pixels.keep(item, params.offsets[0], Combine::Mean, kWeights[sample], kDepths[sample], value.data(), 1);
        for (std::size_t channel = 1; channel < kChannels; ++channel) {
            params.pixels.keep(item, params.offsets[channel], kCombines[channel], kWeights[sample], kDepths[sample], value.data(), 2);
        }
    }
}

/** One pass of combined(), each sample's value moved by the offset given. **/
void pass(CpuDevice & device, film::Film & film, float offset) {
    film.pass([&](const kernel::film::Pixels & pixels) {
        Params params { .pixels = pixels, .offset = offset };
        for (std::size_t channel = 0; channel < kChannels; ++channel) {
            params.offsets[channel] = film.offset(kNames[channel]);
        }
        device.launch(2, 1, params, &combined);
    });
}

// each combine keeps what its name says of four samples. Two samples share the least depth, and
// the closest keeps the first of them
BOOST_AUTO_TEST_CASE(moya_filmcombine_each_combine_keeps_its_share_of_the_samples_test) {
    CpuDevice device;
    film::Film film(device, { 2, 1 }, channels());
    pass(device, film, 0.0F);
    const film::Snapshot snapshot = film.snapshot();
    // the closest holds its key after its two components
    BOOST_TEST(snapshot.stride == 4U + 1U + 2U + 3U + 2U + 2U);

    float weighted = 0.0F;
    float total = 0.0F;
    for (int sample = 0; sample < kSamples; ++sample) {
        weighted += kWeights[sample] * kValues[sample];
        total += kWeights[sample];
    }
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("mean")) == weighted / total, boost::test_tools::tolerance(0.0001F));
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("sum")) == 11.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("sum") + 1U) == -11.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("closest")) == -1.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("closest") + 1U) == 1.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("closest") + 2U) == 1.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("least")) == -1.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("least") + 1U) == -6.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("most")) == 6.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("most") + 1U) == 1.0F);
}

// a pixel no sample reached holds zero in every channel, though a least starts infinite, a most
// minus infinity and a closest's key NaN
BOOST_AUTO_TEST_CASE(moya_filmcombine_a_pixel_no_sample_reached_holds_zero_test) {
    CpuDevice device;
    film::Film film(device, { 2, 1 }, channels());
    pass(device, film, 0.0F);
    const film::Snapshot snapshot = film.snapshot();
    for (std::uint32_t offset = 0; offset < snapshot.stride; ++offset) {
        BOOST_TEST(snapshot.at(1, 0, offset) == 0.0F, "float " << offset);
    }
}

// the second pass's samples are at the same depths as the first's, so the closest keeps the first
// pass's sample, whose index is lower. Its values are greater, so the least keeps the first's and
// the most takes the second's
BOOST_AUTO_TEST_CASE(moya_filmcombine_a_later_pass_keeps_the_earlier_closest_test) {
    CpuDevice device;
    film::Film film(device, { 2, 1 }, channels());
    pass(device, film, 0.0F);
    pass(device, film, 10.0F);
    const film::Snapshot snapshot = film.snapshot();
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("closest")) == -1.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("least")) == -1.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("most")) == 16.0F);
    BOOST_TEST(snapshot.at(0, 0, snapshot.offset("sum")) == 11.0F + 11.0F + 40.0F);
}

// a film read back from a checkpoint keeps the closest's key, so its next pass keeps the first
// pass's closest as the film it was taken from does
BOOST_AUTO_TEST_CASE(moya_filmcombine_a_checkpoint_keeps_the_closest_key_test) {
    CpuDevice device;
    film::Film original(device, { 2, 1 }, channels());
    pass(device, original, 0.0F);
    boost::filesystem::create_directories("data_out");
    const std::string path = "data_out/filmcombine-checkpoint.exr";
    v3d::moya::output::writeCheckpoint(path, original.state(), 1U);

    film::Film restored(device, { 2, 1 }, channels());
    restored.restore(v3d::moya::output::readCheckpoint(path, channels(), 1U));
    pass(device, original, 10.0F);
    pass(device, restored, 10.0F);
    const film::Snapshot want = original.snapshot();
    const film::Snapshot got = restored.snapshot();
    BOOST_TEST(got.values == want.values, boost::test_tools::per_element());
    BOOST_TEST(got.at(0, 0, got.offset("closest")) == -1.0F);
}

// a state is of other channels when a channel of the same name is combined another way
BOOST_AUTO_TEST_CASE(moya_filmcombine_refuses_a_state_of_another_combine_test) {
    CpuDevice device;
    const film::Film closest(device, { 2, 1 }, { { .name = "depth", .components = 1, .combine = Combine::Closest } });
    film::Film least(device, { 2, 1 }, { { .name = "depth", .components = 1, .combine = Combine::Least } });
    BOOST_CHECK_THROW(least.restore(closest.state()), std::invalid_argument);
}

};  // namespace
