/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/FaceMetrics.h>

#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

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
     * @return a face's measures at a size in pixels and at normalized coordinates
     **/
    const FaceMetrics& of(const boost::shared_ptr<Face>& face, float size, const std::vector<std::int32_t>& coordinates);

 private:
    std::map<std::tuple<const Face*, float, std::vector<std::int32_t>>, FaceMetrics> metrics_;
};

};  // namespace v3d::font::internal
