/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/integrate/internal/Crop.h>

#include <api/log/Logger.h>

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <vector>

namespace v3d::moya::integrate::internal {

namespace {

/** The first pixel of a side at or after a fraction of it, clamped to [0, 1]. **/
std::uint32_t edge(float fraction, std::uint32_t side) {
    const float placed = std::ceil(std::clamp(fraction, 0.0F, 1.0F) * static_cast<float>(side));
    return static_cast<std::uint32_t>(placed);  // checked: fraction is clamped to [0, 1], so placed lies in [0, side]
}

};  // namespace

Window windowOf(const std::vector<float> & crop, const glm::uvec2 & image, const boost::shared_ptr<log::Logger> & logger) {
    const Window whole { .origin = glm::uvec2 { 0U }, .size = image };
    const bool numbers = crop.size() == 4U && std::none_of(crop.begin(), crop.end(), [](float value) { return std::isnan(value); });
    if (!numbers) {
        if (logger) {
            logger->get()->warn("the crop window is not four numbers, so the whole image is rendered");
        }
        return whole;
    }
    const glm::uvec2 lower { edge(crop[0], image.x), edge(crop[2], image.y) };
    const glm::uvec2 upper { edge(crop[1], image.x), edge(crop[3], image.y) };
    if (upper.x <= lower.x || upper.y <= lower.y) {
        if (logger) {
            logger->get()->warn("the crop window covers no pixel, so the whole image is rendered");
        }
        return whole;
    }
    return Window { .origin = lower, .size = upper - lower };
}

};  // namespace v3d::moya::integrate::internal
