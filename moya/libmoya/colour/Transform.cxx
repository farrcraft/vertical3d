/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/colour/Transform.h>

#include <moya/libmoya/colour/internal/Processor.h>
#include <OpenColorIO/OpenColorIO.h>

#include <cstddef>
#include <utility>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::colour {

namespace {

namespace ocio = OCIO_NAMESPACE;

};  // namespace

Transform::Transform(boost::shared_ptr<const internal::Processor> processor) : processor_(std::move(processor)) {
}

glm::vec3 Transform::apply(const glm::vec3 & rgb) const {
    float pixel[3] = { rgb.r, rgb.g, rgb.b };
    processor_->cpu()->applyRGB(pixel);
    return { pixel[0], pixel[1], pixel[2] };
}

// NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the count, then the stride
void Transform::apply(float * values, std::size_t pixels, std::size_t stride) const {
    if (pixels == 0) {
        return;
    }
    const auto bytes = static_cast<std::ptrdiff_t>(stride * sizeof(float));
    // OpenColorIO takes the image's width as a long
    const auto width = static_cast<long>(pixels);  // NOLINT(runtime/int)
    const ocio::PackedImageDesc image(values, width, 1, 3, ocio::BIT_DEPTH_F32, sizeof(float), bytes, bytes * static_cast<std::ptrdiff_t>(pixels));
    processor_->cpu()->apply(image);
}

};  // namespace v3d::moya::colour
