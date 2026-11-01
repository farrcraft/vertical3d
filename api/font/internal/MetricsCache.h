/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/FaceMetrics.h>

#include <map>
#include <utility>

#include <boost/shared_ptr.hpp>

namespace v3d::font {
class Face;
};  // namespace v3d::font

namespace v3d::font::internal {

/**
 * The faces' measures at the sizes one layout meets, each read once.
 **/
class MetricsCache {
 public:
    /**
     * @return a face's measures at a size in pixels
     **/
    const FaceMetrics& of(const boost::shared_ptr<Face>& face, float size);

 private:
    std::map<std::pair<const Face*, float>, FaceMetrics> metrics_;
};

};  // namespace v3d::font::internal
