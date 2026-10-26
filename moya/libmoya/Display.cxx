/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Display.h"

#include <api/image/Factory.h>
#include <api/log/Logger.h>
#include <api/render/offline/FrameBuffer.h>
#include <api/render/offline/rib/Requests.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <string>

#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

#include "FrameBuffer.h"

namespace v3d::moya {

void Display::request(const v3d::render::offline::rib::Display & display) {
    name_ = display.name;
    type_ = display.type;
    mode_ = display.mode;
}

const std::string & Display::name() const {
    return name_;
}

bool Display::crop(const v3d::render::offline::rib::CropWindow & window) {
    v3d::render::offline::rib::CropWindow held;
    held.xmin = std::clamp(window.xmin, 0.0F, 1.0F);
    held.xmax = std::clamp(window.xmax, 0.0F, 1.0F);
    held.ymin = std::clamp(window.ymin, 0.0F, 1.0F);
    held.ymax = std::clamp(window.ymax, 0.0F, 1.0F);
    // the negated test also refuses a bound that is not a number
    if (!(held.xmin < held.xmax) || !(held.ymin < held.ymax)) {
        return false;
    }
    crop_ = held;
    return true;
}

glm::uvec4 Display::cropped(unsigned int width, unsigned int height) const {
    const auto first = [](unsigned int size, float fraction) {
        return static_cast<unsigned int>(std::ceil(static_cast<float>(size) * fraction));  // checked: the fraction is held to [0, 1]
    };
    return glm::uvec4(first(width, crop_.xmin), first(width, crop_.xmax), first(height, crop_.ymin), first(height, crop_.ymax));
}

void Display::finish(v3d::render::offline::FrameBuffer* planes, const boost::shared_ptr<v3d::log::Logger> & logger) const {
    const glm::uvec4 window = cropped(planes->width(), planes->height());
    for (unsigned int row = 0; row < planes->height(); row++) {
        for (unsigned int column = 0; column < planes->width(); column++) {
            if (column >= window.x && column < window.y && row >= window.z && row < window.w) {
                continue;
            }
            for (unsigned int plane = 0; plane < planes->planes(); plane++) {
                planes->value(plane, column, row, plane == FrameBuffer::DEPTH ? std::numeric_limits<float>::max() : 0.0F);
            }
        }
    }

    // a display named nothing, or one that is not a file, leaves the samples in the planes
    // rather than writing them
    if (name_.empty() || type_ != "file") {
        return;
    }
    // only the three colour channels are written, whatever the display mode. The coverage and
    // depth planes stay in the framebuffer
    v3d::image::Factory factory(logger);
    if (!factory.write(name_, planes->image(FrameBuffer::CHANNELS))) {
        logger->get()->error("the picture could not be written to '{}'", name_);
    }
}

};  // namespace v3d::moya
