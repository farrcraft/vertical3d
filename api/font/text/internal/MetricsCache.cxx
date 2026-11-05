/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MetricsCache.h"

#include <api/font/face/Face.h>
#include <api/font/face/Metrics.h>

#include <cstdint>
#include <tuple>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::font::text::internal {

const face::Metrics& MetricsCache::of(const boost::shared_ptr<face::Face>& face, float size, const std::vector<std::int32_t>& coordinates) {
    auto key = std::make_tuple(face.get(), size, coordinates);
    auto found = metrics_.find(key);
    if (found == metrics_.end()) {
        found = metrics_.emplace(std::move(key), face->metrics(size, coordinates)).first;
    }
    return found->second;
}

};  // namespace v3d::font::text::internal
