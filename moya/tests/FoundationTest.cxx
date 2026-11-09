/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/offline/film/Sampling.h>
#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/colour/RgbTables.h>
#include <moya/libmoya/colour/Transform.h>
#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/CpuDevice.h>
#include <moya/libmoya/film/Film.h>
#include <moya/libmoya/film/Filter.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/film/FilterSample.h>
#include <moya/libmoya/kernel/film/PixelFilter.h>
#include <moya/libmoya/kernel/film/Pixels.h>
#include <moya/libmoya/kernel/Launch.h>
#include <moya/libmoya/kernel/sample/ZSobol.h>
#include <moya/libmoya/kernel/spectrum/Acescg.h>
#include <moya/libmoya/kernel/spectrum/Rgb.h>
#include <moya/libmoya/kernel/spectrum/RgbTable.h>
#include <moya/libmoya/kernel/spectrum/Spectrum.h>
#include <moya/libmoya/kernel/spectrum/Wavelengths.h>
#include <moya/libmoya/output/Exr.h>
#include <moya/libmoya/output/ExrPart.h>
#include <moya/libmoya/output/Png.h>
#include <Imath/ImathBox.h>
#include <OpenEXR/ImfFrameBuffer.h>
#include <OpenEXR/ImfHeader.h>
#include <OpenEXR/ImfInputFile.h>
#include <OpenEXR/ImfPixelType.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/test/unit_test.hpp>

#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

#include "Chart.h"
#include "Reference.h"

namespace {

namespace kernel = v3d::moya::kernel;
namespace film = v3d::moya::film;
namespace output = v3d::moya::output;
typedef v3d::moya::device::CpuDevice CpuDevice;

/** The chart's columns and rows of patches. **/
constexpr std::uint32_t kColumns = 6;
constexpr std::uint32_t kRows = 4;

/** A patch's cell, in pixels, of which the first kGutter rows and columns are black. **/
constexpr std::uint32_t kCell = 16;
constexpr std::uint32_t kGutter = 2;

/** The picture: the cells, and a gutter along the right and the bottom. **/
constexpr std::uint32_t kWidth = (kColumns * kCell) + kGutter;
constexpr std::uint32_t kHeight = (kRows * kCell) + kGutter;
constexpr std::size_t kPixels = static_cast<std::size_t>(kWidth) * kHeight;

constexpr std::uint32_t kSamples = 64;

constexpr const char * kReference = "data/foundation-chart.png";
constexpr const char * kRendered = "data_out/foundation-chart.png";

/** What the chart's kernel function reads. **/
struct Chart {
    kernel::film::Pixels pixels;
    kernel::film::PixelFilter filter;
    kernel::sample::ZSobol sampler;
    kernel::spectrum::RgbTable table;
    kernel::Array<glm::vec3> patches;
};

/** The ACEScg colour at a point, in pixels from the picture's top left: a patch's, or black. **/
glm::vec3 colourAt(const Chart & chart, const glm::vec2 & point) {
    if (point.x < 0.0F || point.y < 0.0F) {
        return glm::vec3(0.0F);
    }
    const auto column = static_cast<std::uint32_t>(point.x) / kCell;
    const auto row = static_cast<std::uint32_t>(point.y) / kCell;
    const float across = point.x - static_cast<float>(column * kCell);
    const float down = point.y - static_cast<float>(row * kCell);
    if (column >= kColumns || row >= kRows || across < static_cast<float>(kGutter) || down < static_cast<float>(kGutter)) {
        return glm::vec3(0.0F);
    }
    return chart.patches[(row * kColumns) + column];
}

/**
 * Renders one pixel of the chart: each sample draws its place from the filter and its wavelengths,
 * and adds the reflectance of the patch it lands on, lit by ACES white.
 **/
void render(const Chart & chart, kernel::Item item) {
    const glm::uvec2 pixel { item.x, item.y };
    for (std::uint32_t index = 0; index < kSamples; ++index) {
        const kernel::film::FilterSample placed = chart.filter.sample(chart.sampler.get2D(pixel, index, 0));
        const auto wavelengths = kernel::spectrum::SampledWavelengths::visible(chart.sampler.get1D(pixel, index, 1));
        const glm::vec2 point = glm::vec2(pixel) + 0.5F + placed.offset;
        const kernel::spectrum::SampledSpectrum radiance =
            kernel::spectrum::reflectance(chart.table, colourAt(chart, point), wavelengths) * kernel::spectrum::acesWhite(wavelengths);
        chart.pixels.add(item, placed.weight, radiance, wavelengths);
    }
}

/** The chart rendered on a device of the given threads, as a snapshot. **/
film::Snapshot rendered(unsigned int threads) {
    CpuDevice device(threads);
    const std::vector<glm::vec3> colours = v3d::moya::chart::colours();
    auto patches = device.allocate(colours.size() * sizeof(glm::vec3));
    device.upload(*patches, colours.data(), colours.size() * sizeof(glm::vec3));
    const film::Filter filter(device, v3d::render::offline::film::Filter::Gaussian, { 2.0F, 2.0F });
    film::Film picture(device, { kWidth, kHeight }, {});
    picture.pass([&](const kernel::film::Pixels & pixels) {
        const Chart chart { .pixels = pixels, .filter = filter.view(), .sampler = kernel::sample::ZSobol(kSamples, { kWidth, kHeight }, 1, 0),
            .table = v3d::moya::colour::acescgTable(), .patches = v3d::moya::device::view<glm::vec3>(*patches) };
        device.launch(kWidth, kHeight, chart, &render);
    });
    return picture.snapshot();
}

/** The snapshot in the working space, written as one float part. **/
void writeExr(const std::string & path, film::Snapshot snapshot, const v3d::moya::colour::Config & config) {
    config.output().apply(snapshot.values.data(), snapshot.values.size() / snapshot.stride, snapshot.stride);
    output::writeExr(path, snapshot, { { .name = "", .outputs = { std::string(output::kBeauty) }, .precision = output::Precision::Float } },
        { .colourSpace = config.workingInteropId() });
}

/** A file's bytes. **/
std::vector<char> bytesOf(const std::string & path) {
    std::ifstream file(path, std::ios::binary);
    return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}

/** Where a case writes its files, made empty when it starts. **/
std::string scratch(const std::string & name) {
    const boost::filesystem::path path = boost::filesystem::temp_directory_path() / ("v3d_foundation_" + name);
    boost::system::error_code ignored;
    boost::filesystem::remove_all(path, ignored);
    boost::filesystem::create_directories(path);
    return path.string();
}

// ACEScg to ACEScg changes nothing, which leaves the 8-bit encoding. A value below zero is zero,
// one above one is 255 and a NaN is zero. The rest round to the nearest step
BOOST_AUTO_TEST_CASE(moya_output_a_displayed_value_is_clamped_rounded_and_never_nan_test) {
    const film::Snapshot snapshot { .resolution = { 2, 1 }, .passes = 1, .stride = 4, .channels = { { .name = "depth", .components = 1 } },
        .offsets = { 3 }, .values = { -1.0F, 2.0F, std::nanf(""), 9.0F, 0.5F, 0.25F, 1.0F, 9.0F } };
    const std::vector<std::uint8_t> bytes = output::displayed(snapshot, v3d::moya::colour::Config().input("ACEScg"));
    BOOST_TEST(bytes == std::vector<std::uint8_t>({ 0, 255, 0, 128, 64, 255 }), boost::test_tools::per_element());
}

#ifndef MOYA_RGB
// the chart through the sRGB display's ACES 2.0 SDR view. An RGB build draws no wavelengths, so its
// picture is not this one
BOOST_AUTO_TEST_CASE(moya_foundation_a_colour_chart_renders_its_reference_test) {
    const v3d::moya::colour::Config config;
    const std::string display = config.defaultDisplay();
    const std::vector<std::uint8_t> bytes = output::displayed(rendered(0), config.display(display, config.defaultView(display)));
    const auto image = boost::make_shared<v3d::image::Image>(glm::uvec2(kWidth, kHeight), 24);
    std::memcpy(image->data(), bytes.data(), bytes.size());
    v3d::moya::reference::check(image, kReference, kRendered);
}
#endif

// the middle of each patch, away from the filter's reach into the gutters, is the patch's ACEScg
// colour within the fit's error and the noise of 64 samples a pixel
BOOST_AUTO_TEST_CASE(moya_foundation_the_charts_exr_holds_each_patchs_acescg_test) {
    const std::string path = scratch("patches") + "/chart.exr";
    const v3d::moya::colour::Config config;
    writeExr(path, rendered(0), config);

    Imf::InputFile file(path.c_str());
    std::vector<float> channels[3] = { std::vector<float>(kPixels), std::vector<float>(kPixels),
        std::vector<float>(kPixels) };
    Imf::FrameBuffer buffer;
    const char * const names[3] = { "R", "G", "B" };
    for (int i = 0; i < 3; ++i) {
        buffer.insert(names[i], Imf::Slice(Imf::FLOAT, reinterpret_cast<char *>(channels[i].data()), sizeof(float), sizeof(float) * kWidth));
    }
    file.setFrameBuffer(buffer);
    file.readPixels(0, static_cast<int>(kHeight) - 1);

    const std::vector<glm::vec3> colours = v3d::moya::chart::colours();
    for (std::uint32_t row = 0; row < kRows; ++row) {
        for (std::uint32_t column = 0; column < kColumns; ++column) {
            glm::vec3 mean(0.0F);
            for (std::uint32_t y = 0; y < 8; ++y) {
                for (std::uint32_t x = 0; x < 8; ++x) {
                    const std::size_t at = (((row * kCell) + kGutter + 3 + y) * kWidth) + (column * kCell) + kGutter + 3 + x;
                    mean += glm::vec3(channels[0][at], channels[1][at], channels[2][at]);
                }
            }
            mean /= 64.0F;
            const glm::vec3 & want = colours[(row * kColumns) + column];
            BOOST_TEST_CONTEXT("patch " << column << ", " << row) {
                for (int i = 0; i < 3; ++i) {
                    BOOST_TEST(std::fabs(mean[i] - want[i]) < 0.015F);
                }
            }
        }
    }
}

BOOST_AUTO_TEST_CASE(moya_foundation_one_thread_and_every_thread_write_the_same_files_test) {
    const std::string directory = scratch("threads");
    const v3d::moya::colour::Config config;
    const std::string display = config.defaultDisplay();
    const v3d::moya::colour::Transform shown = config.display(display, config.defaultView(display));
    const auto logger = boost::make_shared<v3d::log::Logger>();
    const film::Snapshot one = rendered(1);
    const film::Snapshot every = rendered(0);
    writeExr(directory + "/one.exr", one, config);
    writeExr(directory + "/every.exr", every, config);
    output::writePng(directory + "/one.png", one, shown, logger);
    output::writePng(directory + "/every.png", every, shown, logger);
    BOOST_TEST(!bytesOf(directory + "/one.exr").empty());
    BOOST_TEST((bytesOf(directory + "/one.exr") == bytesOf(directory + "/every.exr")));
    BOOST_TEST(!bytesOf(directory + "/one.png").empty());
    BOOST_TEST((bytesOf(directory + "/one.png") == bytesOf(directory + "/every.png")));
}

};  // namespace
