/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/light/internal/Profiles.h>

#include <api/log/Logger.h>
#include <moya/libmoya/kernel/Array.h>
#include <moya/libmoya/kernel/geometry/Hit.h>
#include <moya/libmoya/kernel/light/Profile.h>
#include <moya/libmoya/kernel/light/SphericalRectangle.h>
#include <moya/libmoya/light/Ies.h>

#include <cmath>
#include <cstdint>
#include <fstream>
#include <ios>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::light::internal {

namespace {

/** The bands of equal area, and the steps around each, that a profile's mean is summed over. **/
constexpr int kBands = 256;
constexpr int kSteps = 512;

};  // namespace

Profiles::Profiles(boost::shared_ptr<log::Logger> logger) : logger_(std::move(logger)) {
}

std::uint32_t Profiles::load(const std::string & path, float angleScale, bool normalize) {
    auto found = byPath_.find(path);
    if (found == byPath_.end()) {
        kernel::light::Profile read;
        Photometry photometry;
        std::string why;
        std::ifstream stream(path, std::ios::binary);
        const std::string text = stream ? std::string((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>()) : std::string();
        if (!stream) {
            why = "it cannot be opened";
        } else if (parseIes(text, photometry, why)) {
            read.vertical = static_cast<std::uint32_t>(photometry_.size());
            read.verticalCount = static_cast<std::uint32_t>(photometry.vertical.size());
            photometry_.insert(photometry_.end(), photometry.vertical.begin(), photometry.vertical.end());
            read.horizontal = static_cast<std::uint32_t>(photometry_.size());
            read.horizontalCount = static_cast<std::uint32_t>(photometry.horizontal.size());
            photometry_.insert(photometry_.end(), photometry.horizontal.begin(), photometry.horizontal.end());
            read.candela = static_cast<std::uint32_t>(photometry_.size());
            photometry_.insert(photometry_.end(), photometry.candela.begin(), photometry.candela.end());
        }
        if (read.verticalCount == 0U && logger_) {
            logger_->get()->warn("the IES file {} cannot be read, since {}, so its light is not shaped by it", path, why);
        }
        found = byPath_.emplace(path, read).first;
    }
    kernel::light::Profile made = found->second;
    if (made.verticalCount == 0U) {
        return kernel::geometry::kNoPrimitive;
    }
    if (normalize) {
        const float average = meanOf(made);
        made.scale = average > 0.0F ? 1.0F / average : 0.0F;
    }
    made.angleScale = angleScale;
    profiles_.push_back(made);
    return static_cast<std::uint32_t>(profiles_.size() - 1U);
}

float Profiles::mean(std::uint32_t profile) const {
    return meanOf(profiles_[profile]);
}

const std::vector<kernel::light::Profile> & Profiles::profiles() const {
    return profiles_;
}

const std::vector<float> & Profiles::photometry() const {
    return photometry_;
}

float Profiles::meanOf(const kernel::light::Profile & profile) const {
    std::vector<float> held = photometry_;
    const kernel::Array<float> values { .data = held.data(), .size = static_cast<std::uint32_t>(held.size()) };
    double sum = 0.0;
    for (int band = 0; band < kBands; ++band) {
        // heights spread evenly over [-1, 1] cover the sphere in bands of equal area
        const float z = -1.0F + ((static_cast<float>(band) + 0.5F) * (2.0F / static_cast<float>(kBands)));
        const float across = std::sqrt(1.0F - (z * z));
        for (int step = 0; step < kSteps; ++step) {
            const float phi = (static_cast<float>(step) + 0.5F) * (2.0F * kernel::light::kHalfTurn / static_cast<float>(kSteps));
            sum += kernel::light::profileShare(profile, values, glm::vec3 { across * std::cos(phi), across * std::sin(phi), z });
        }
    }
    return static_cast<float>(sum / (static_cast<double>(kBands) * kSteps));
}

};  // namespace v3d::moya::light::internal
