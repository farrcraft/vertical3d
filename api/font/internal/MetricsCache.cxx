/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "MetricsCache.h"

#include <api/font/Face.h>
#include <api/font/FaceMetrics.h>

#include <utility>

#include <boost/shared_ptr.hpp>

namespace v3d::font::internal {

const FaceMetrics& MetricsCache::of(const boost::shared_ptr<Face>& face, float size) {
    const auto key = std::make_pair(face.get(), size);
    auto found = metrics_.find(key);
    if (found == metrics_.end()) {
        found = metrics_.emplace(key, face->metrics(size)).first;
    }
    return found->second;
}

};  // namespace v3d::font::internal
