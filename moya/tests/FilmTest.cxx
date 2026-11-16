/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/film/Filter.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/film/State.h>
#include <moya/libmoya/kernel/film/FilterSample.h>
#include <moya/libmoya/kernel/film/PixelFilter.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Colorimetry.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <api/rib/Sampling.h>

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

namespace {

namespace kernel = v3d::moya::kernel;
namespace film = v3d::moya::film;
namespace rib = v3d::rib;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** The samples weighted() adds to each pixel. **/
constexpr int kWeighted = 3;

/** Each of weighted()'s samples: its weight, a random number for its wavelengths, its radiance. **/
constexpr float kWeights[kWeighted] = { 1.0F, 2.0F, 0.5F };
constexpr float kLambdas[kWeighted] = { 0.1F, 0.5F, 0.8F };
constexpr float kRadiances[kWeighted] = { 1.0F, 3.0F, 0.25F };

/** The samples halved() adds to each pixel, the first half of them to the half beauty. **/
constexpr std::uint32_t kHalved = 16;

/** What a kernel function writes, and where the film's channels are. **/
struct Params {
    kernel::film::Pixels pixels;
    std::uint32_t depth { 0 };
    std::uint32_t albedo { 0 };
    std::uint32_t pass { 0 };
    kernel::sample::ZSobol sampler;
};

/** A depth and an albedo for each of a pixel's samples, which differ by pixel and by sample. **/
float depthOf(kernel::Item item, int sample) {
    return static_cast<float>(item.x + (2U * item.y)) + static_cast<float>(sample);
}

glm::vec3 albedoOf(kernel::Item item, int sample) {
    return glm::vec3(0.1F * static_cast<float>(sample), static_cast<float>(item.x), static_cast<float>(item.y));
}

/** The beauty a sample of weighted() adds, before its weight. **/
glm::vec3 beautyOf(int sample) {
    const auto wavelengths = kernel::spectrum::SampledWavelengths::visible(kLambdas[sample]);
    const kernel::spectrum::SampledSpectrum radiance(kRadiances[sample]);
    return kernel::spectrum::acescgFromXyz(kernel::spectrum::toXyz(radiance, wavelengths));
}

/** Adds kWeighted samples to the pixel, each with its own weight. **/
void weighted(const Params & params, kernel::Item item) {
    for (int sample = 0; sample < kWeighted; ++sample) {
        const auto wavelengths = kernel::spectrum::SampledWavelengths::visible(kLambdas[sample]);
        const kernel::spectrum::SampledSpectrum radiance(kRadiances[sample]);
        params.pixels.add(item, kWeights[sample], radiance, wavelengths);
        params.pixels.addChannel(item, params.depth, kWeights[sample], depthOf(item, sample));
        params.pixels.addChannel(item, params.albedo, kWeights[sample], albedoOf(item, sample));
    }
}

/** Adds one sample of weight one, whose depth is the pass's number counted from one. **/
void numbered(const Params & params, kernel::Item item) {
    params.pixels.add(item, 1.0F, kernel::spectrum::SampledSpectrum(0.0F), kernel::spectrum::SampledWavelengths::visible(0.0F));
    params.pixels.addChannel(item, params.depth, 1.0F, static_cast<float>(params.pass + 1U));
}

/** Adds 64 samples whose wavelengths, weights and depths are drawn from the sampler. **/
void sampled(const Params & params, kernel::Item item) {
    const glm::uvec2 pixel { item.x, item.y };
    for (std::uint32_t index = 0; index < 64; ++index) {
        const float weight = params.sampler.get1D(pixel, index, 0) - 0.25F;
        const auto wavelengths = kernel::spectrum::SampledWavelengths::visible(params.sampler.get1D(pixel, index, 1));
        const kernel::spectrum::SampledSpectrum radiance(params.sampler.get1D(pixel, index, 2) * 10.0F);
        params.pixels.add(item, weight, radiance, wavelengths);
        params.pixels.addChannel(item, params.depth, weight, params.sampler.get1D(pixel, index, 3));
    }
}

/**
 * Whether halved() makes a pixel noisy: one in the two right-hand columns, or at the left of the
 * third row.
 **/
bool noisy(kernel::Item item) {
    return item.x >= 3U || (item.x == 0U && item.y == 2U);
}

/**
 * Adds kHalved samples to a pixel that has not stopped, the first half to the half beauty too.
 * Each sample of a flat pixel is one. A noisy pixel's are a half in the first half and one and a
 * half in the second.
 **/
void halved(const Params & params, kernel::Item item) {
    if (params.pixels.done(item)) {
        return;
    }
    for (std::uint32_t sample = 0; sample < kHalved; ++sample) {
        const bool half = sample < kHalved / 2U;
        float radiance = 1.0F;
        if (noisy(item)) {
            radiance = half ? 0.5F : 1.5F;
        }
        params.pixels.add(item, 1.0F, kernel::spectrum::SampledSpectrum(radiance), kernel::spectrum::SampledWavelengths::visible(0.5F), half);
    }
}

/** A film with a depth and an albedo channel. **/
film::Film withChannels(CpuDevice & device, const glm::uvec2 & resolution) {
    return film::Film(device, resolution, { { .name = "depth", .components = 1 }, { .name = "albedo", .components = 3 } });
}

/** The parameters for a film. **/
Params paramsFor(const film::Film & film, const kernel::film::Pixels & pixels) {
    return Params { .pixels = pixels, .depth = film.offset("depth"), .albedo = film.offset("albedo"), .pass = 0, .sampler = {} };
}

/** The film after one pass of sampled(), on a device of the given threads. **/
film::Snapshot sampledOn(unsigned int threads) {
    CpuDevice device(threads);
    film::Film film = withChannels(device, { 67, 45 });
    film.pass([&](const kernel::film::Pixels & pixels) {
        Params params = paramsFor(film, pixels);
        params.sampler = kernel::sample::ZSobol(64, { 67, 45 }, 9, 0);
        device.launch(67, 45, params, &sampled);
    });
    return film.snapshot();
}

/**
 * The integral of a filter over each of a grid of bins across its extent, from its RI formula at
 * 8 by 8 points of each bin.
 **/
std::vector<float> exactBins(rib::Filter kind, const glm::vec2 & width, int bins) {
    std::vector<float> out(static_cast<std::size_t>(bins) * bins, 0.0F);
    const glm::vec2 bin = width / static_cast<float>(bins);
    constexpr int kPoints = 8;
    for (int row = 0; row < bins; ++row) {
        for (int column = 0; column < bins; ++column) {
            float sum = 0.0F;
            for (int y = 0; y < kPoints; ++y) {
                for (int x = 0; x < kPoints; ++x) {
                    const glm::vec2 at { (static_cast<float>(column) + ((static_cast<float>(x) + 0.5F) / kPoints)) * bin.x,
                        (static_cast<float>(row) + ((static_cast<float>(y) + 0.5F) / kPoints)) * bin.y };
                    sum += film::Filter::weight(kind, at - (width * 0.5F), width);
                }
            }
            out[(static_cast<std::size_t>(row) * bins) + column] = sum / (kPoints * kPoints) * bin.x * bin.y;
        }
    }
    return out;
}

/** Each bin's sum of the weights of the samples drawn into it, over the number drawn. **/
std::vector<float> drawnBins(const kernel::film::PixelFilter & filter, const glm::vec2 & width, int bins) {
    std::vector<float> out(static_cast<std::size_t>(bins) * bins, 0.0F);
    constexpr int kGrid = 1024;
    for (int y = 0; y < kGrid; ++y) {
        for (int x = 0; x < kGrid; ++x) {
            const glm::vec2 u { (static_cast<float>(x) + 0.5F) / kGrid, (static_cast<float>(y) + 0.5F) / kGrid };
            const kernel::film::FilterSample drawn = filter.sample(u);
            const glm::vec2 place = ((drawn.offset / width) + 0.5F) * static_cast<float>(bins);
            const int column = std::clamp(static_cast<int>(place.x), 0, bins - 1);
            const int row = std::clamp(static_cast<int>(place.y), 0, bins - 1);
            out[(static_cast<std::size_t>(row) * bins) + column] += drawn.weight;
        }
    }
    for (float & value : out) {
        value /= kGrid * kGrid;
    }
    return out;
}

BOOST_AUTO_TEST_CASE(moya_film_a_pixel_is_the_weighted_mean_of_its_samples_test) {
    CpuDevice device;
    film::Film film = withChannels(device, { 5, 3 });
    film.pass([&](const kernel::film::Pixels & pixels) { device.launch(5, 3, paramsFor(film, pixels), &weighted); });
    const film::Snapshot snapshot = film.snapshot();
    BOOST_TEST(snapshot.passes == 1U);
    BOOST_TEST(snapshot.stride == 8U);

    const float total = kWeights[0] + kWeights[1] + kWeights[2];
    for (std::uint32_t y = 0; y < 3; ++y) {
        for (std::uint32_t x = 0; x < 5; ++x) {
            glm::vec3 beauty(0.0F);
            float depth = 0.0F;
            glm::vec3 albedo(0.0F);
            for (int sample = 0; sample < kWeighted; ++sample) {
                beauty += kWeights[sample] * beautyOf(sample);
                depth += kWeights[sample] * depthOf({ x, y }, sample);
                albedo += kWeights[sample] * albedoOf({ x, y }, sample);
            }
            beauty /= total;
            BOOST_TEST_CONTEXT("pixel " << x << ", " << y) {
                for (int i = 0; i < 3; ++i) {
                    BOOST_TEST(snapshot.at(x, y, static_cast<std::uint32_t>(i)) == beauty[i], boost::test_tools::tolerance(0.0001F));
                    BOOST_TEST(snapshot.at(x, y, film.offset("albedo") + static_cast<std::uint32_t>(i)) == albedo[i] / total,
                        boost::test_tools::tolerance(0.0001F));
                }
                BOOST_TEST(snapshot.at(x, y, film.offset("depth")) == depth / total, boost::test_tools::tolerance(0.0001F));
            }
        }
    }
}

// passes run on one thread while snapshots are taken on another. Each snapshot is the mean of
// the pass numbers so far in every pixel, so no snapshot holds part of a pass
BOOST_AUTO_TEST_CASE(moya_film_a_snapshot_holds_whole_passes_test) {
    constexpr std::uint32_t kPasses = 40;
    CpuDevice device;
    film::Film film = withChannels(device, { 256, 256 });
    std::atomic<bool> done { false };
    std::thread sampling([&] {
        for (std::uint32_t pass = 0; pass < kPasses; ++pass) {
            film.pass([&](const kernel::film::Pixels & pixels) {
                Params params = paramsFor(film, pixels);
                params.pass = pass;
                device.launch(256, 256, params, &numbered);
            });
        }
        done = true;
    });

    int taken = 0;
    int partial = 0;
    while (!done) {
        const film::Snapshot snapshot = film.snapshot();
        const float expected = snapshot.passes == 0 ? 0.0F : (static_cast<float>(snapshot.passes) + 1.0F) / 2.0F;
        for (std::uint32_t y = 0; y < 256; ++y) {
            for (std::uint32_t x = 0; x < 256; ++x) {
                partial += snapshot.at(x, y, film.offset("depth")) == expected ? 0 : 1;
            }
        }
        ++taken;
    }
    sampling.join();
    BOOST_TEST(taken > 0);
    BOOST_TEST(partial == 0);
    BOOST_TEST(film.snapshot().passes == kPasses);
}

BOOST_AUTO_TEST_CASE(moya_film_is_the_same_on_one_thread_and_on_every_thread_test) {
    const film::Snapshot one = sampledOn(1);
    const film::Snapshot every = sampledOn(0);
    BOOST_TEST(one.values == every.values, boost::test_tools::per_element());
}

// a pixel's noise is how far its half beauty is from its beauty, over the square root of the
// beauty's sum, and is infinite while either holds no weight
BOOST_AUTO_TEST_CASE(moya_film_a_pixels_noise_is_the_distance_of_its_halves_test) {
    // the beauty, the half beauty and its weight, the count, the moments and the weight
    const float apart[] = { 2.0F, 4.0F, 6.0F, 0.5F, 1.0F, 2.0F, 1.0F, 2.0F, 0.0F, 0.0F, 0.0F, 2.0F };
    const float expected = (0.5F + 1.0F + 1.0F) / (1.0e-4F + std::sqrt(6.0F));
    BOOST_TEST(film::Film::noise(apart, 3) == expected, boost::test_tools::tolerance(1.0e-6F));

    const float together[] = { 2.0F, 4.0F, 6.0F, 1.0F, 2.0F, 3.0F, 1.0F, 2.0F, 0.0F, 0.0F, 0.0F, 2.0F };
    BOOST_TEST(film::Film::noise(together, 3) == 0.0F);

    const float noHalf[] = { 2.0F, 4.0F, 6.0F, 0.0F, 0.0F, 0.0F, 0.0F, 2.0F, 0.0F, 0.0F, 0.0F, 2.0F };
    BOOST_TEST(std::isinf(film::Film::noise(noHalf, 3)));
}

// a pixel stops once its noise and its four neighbours' are below the threshold, and takes no
// samples after it stops. Each pixel's samples are counted
BOOST_AUTO_TEST_CASE(moya_film_a_pixel_stops_with_its_neighbours_test) {
    CpuDevice device;
    film::Film film = withChannels(device, { 5, 3 });
    const auto pass = [&]() {
        film.pass([&](const kernel::film::Pixels & pixels) {
            device.launch(5, 3, paramsFor(film, pixels), &halved);
        });
    };
    pass();
    BOOST_TEST(film.snapshot().count(4, 2) == kHalved);
    // the third column and the two pixels beside the noisy corner keep sampling, so three stop
    BOOST_TEST(film.converge(0.01F) == 12U);
    pass();
    const film::Snapshot snapshot = film.snapshot();
    for (std::uint32_t y = 0; y < 3; ++y) {
        for (std::uint32_t x = 0; x < 5; ++x) {
            const bool stopped = (x < 2U && y == 0U) || (x == 1U && y == 1U);
            BOOST_TEST(snapshot.count(x, y) == (stopped ? kHalved : 2U * kHalved), "pixel " << x << ", " << y);
        }
    }
    // a pixel that stopped stays stopped
    BOOST_TEST(film.converge(0.01F) == 12U);
}

// a film restored from another's state holds its floats, its stopped pixels and its passes, and
// the passes after continue from them as they do in the film the state was taken from
BOOST_AUTO_TEST_CASE(moya_film_a_restored_film_continues_as_its_original_test) {
    CpuDevice device;
    film::Film original = withChannels(device, { 5, 3 });
    film::Film restored = withChannels(device, { 5, 3 });
    const auto pass = [&](film::Film & film) {
        film.pass([&](const kernel::film::Pixels & pixels) {
            device.launch(5, 3, paramsFor(film, pixels), &halved);
        });
    };
    pass(original);
    BOOST_TEST(original.converge(0.01F) == 12U);
    const film::State state = original.state();
    BOOST_TEST(state.passes == 1U);
    BOOST_TEST(state.stopped.size() == 15U);
    restored.restore(state);
    pass(original);
    pass(restored);
    const film::Snapshot expected = original.snapshot();
    const film::Snapshot actual = restored.snapshot();
    BOOST_TEST(actual.passes == 2U);
    BOOST_TEST(actual.values == expected.values, boost::test_tools::per_element());
    BOOST_TEST(actual.samples == expected.samples, boost::test_tools::per_element());
    BOOST_TEST(restored.state().stopped == original.state().stopped, boost::test_tools::per_element());
}

BOOST_AUTO_TEST_CASE(moya_film_refuses_a_state_of_another_film_test) {
    CpuDevice device;
    const film::State state = withChannels(device, { 5, 3 }).state();
    BOOST_CHECK_THROW(withChannels(device, { 3, 5 }).restore(state), std::invalid_argument);
    BOOST_CHECK_THROW(film::Film(device, { 5, 3 }, {}).restore(state), std::invalid_argument);
    BOOST_CHECK_THROW(film::Film(device, { 5, 3 }, { { .name = "depth", .components = 2 }, { .name = "albedo", .components = 2 } }).restore(state),
        std::invalid_argument);
    film::State shorter = state;
    shorter.sums.pop_back();
    BOOST_CHECK_THROW(withChannels(device, { 5, 3 }).restore(shorter), std::invalid_argument);
    film::State unflagged = state;
    unflagged.stopped.pop_back();
    BOOST_CHECK_THROW(withChannels(device, { 5, 3 }).restore(unflagged), std::invalid_argument);
}

BOOST_AUTO_TEST_CASE(moya_film_refuses_channels_it_cannot_hold_test) {
    CpuDevice device;
    BOOST_CHECK_THROW(film::Film(device, { 0, 4 }, {}), std::invalid_argument);
    BOOST_CHECK_THROW(film::Film(device, { 4, 4 }, { { .name = "", .components = 1 } }), std::invalid_argument);
    BOOST_CHECK_THROW(film::Film(device, { 4, 4 }, { { .name = "normal", .components = 5 } }), std::invalid_argument);
    BOOST_CHECK_THROW(film::Film(device, { 4, 4 }, { { .name = "depth", .components = 1 }, { .name = "depth", .components = 1 } }),
        std::invalid_argument);
    BOOST_CHECK_THROW(film::Film(device, { 4, 4 }, {}).offset("depth"), std::out_of_range);
}

// samples drawn from a filter, each counted with its weight, land in each part of the filter's
// extent in proportion to the filter's integral over it, negative lobes included
BOOST_AUTO_TEST_CASE(moya_film_filtered_samples_reproduce_each_filters_shape_test) {
    const std::pair<rib::Filter, glm::vec2> filters[] = {
        { rib::Filter::Box, { 2.0F, 2.0F } },
        { rib::Filter::Triangle, { 2.0F, 2.0F } },
        { rib::Filter::CatmullRom, { 4.0F, 4.0F } },
        { rib::Filter::Gaussian, { 2.0F, 3.0F } },
        { rib::Filter::Sinc, { 4.0F, 4.0F } },
    };
    constexpr int kBins = 16;
    CpuDevice device;
    for (const auto & [kind, width] : filters) {
        const film::Filter filter(device, kind, width);
        const std::vector<float> exact = exactBins(kind, width, kBins);
        const std::vector<float> drawn = drawnBins(filter.view(), width, kBins);
        float exactTotal = 0.0F;
        float drawnTotal = 0.0F;
        float largest = 0.0F;
        for (std::size_t i = 0; i < exact.size(); ++i) {
            exactTotal += exact[i];
            drawnTotal += drawn[i];
            largest = std::max(largest, std::fabs(exact[i]));
        }
        BOOST_TEST_CONTEXT(rib::filterName(kind)) {
            for (std::size_t i = 0; i < exact.size(); ++i) {
                BOOST_TEST(std::fabs((drawn[i] / drawnTotal) - (exact[i] / exactTotal)) <= 0.01F * largest / exactTotal);
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_film_refuses_a_filter_width_it_cannot_sample_test) {
    CpuDevice device;
    BOOST_CHECK_THROW(film::Filter(device, rib::Filter::Gaussian, { 0.0F, 2.0F }), std::invalid_argument);
    BOOST_CHECK_THROW(film::Filter(device, rib::Filter::Box, { 2.0F, std::nanf("") }), std::invalid_argument);
}

};  // namespace
