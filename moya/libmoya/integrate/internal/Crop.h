/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <vector>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::integrate::internal {

/** The pixels a render covers: the place of its first in the image, and how many on each side. **/
struct Window {
    glm::uvec2 origin { 0U };
    glm::uvec2 size { 0U };
};

/**
 * The pixels a crop window covers, as RI rounds them: from ceil(side * least) up to and not
 * including ceil(side * most), on each axis. The crop is RI's xmin, xmax, ymin and ymax, each
 * clamped to [0, 1]. A window of no pixels, or one that is not four numbers, is logged, when there
 * is a logger, and the whole image is the window.
 **/
Window windowOf(const std::vector<float> & crop, const glm::uvec2 & image, const boost::shared_ptr<log::Logger> & logger);

};  // namespace v3d::moya::integrate::internal
