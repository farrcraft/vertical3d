/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/denoise/Denoise.h>

#include <api/log/Logger.h>
#include <moya/libmoya/denoise/Denoiser.h>
#include <moya/libmoya/film/Snapshot.h>
#include <moya/libmoya/output/Channel.h>

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::denoise {

namespace {

/** ACEScg's luminance weights, the Y row of its matrix into CIE XYZ. **/
constexpr double kRed = 0.2722287168;
constexpr double kGreen = 0.6740817658;
constexpr double kBlue = 0.0536895174;

/** The least luminance a pixel holds to count as holding light. **/
constexpr double kDark = 1.0e-8;

};  // namespace

float scaleOf(const std::vector<float> & values, std::size_t stride, std::size_t first, std::size_t pixels) {
    double logs = 0.0;
    std::size_t lit = 0;
    for (std::size_t pixel = 0; pixel < pixels; ++pixel) {
        const float * const colour = &values[(pixel * stride) + first];
        const double luminance = (kRed * colour[0]) + (kGreen * colour[1]) + (kBlue * colour[2]);
        if (luminance > kDark) {
            logs += std::log(luminance);
            ++lit;
        }
    }
    return lit == 0 ? 1.0F : static_cast<float>(kKey / std::exp(logs / static_cast<double>(lit)));
}

void denoise(film::Snapshot & snapshot, const std::vector<output::Channel> & channels, bool beauty, const boost::shared_ptr<log::Logger> & logger) {
    Denoiser(logger).into(snapshot, channels, beauty);
}

};  // namespace v3d::moya::denoise
