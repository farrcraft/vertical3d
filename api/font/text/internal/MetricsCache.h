/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/face/Metrics.h>

#include <cstdint>
#include <map>
#include <tuple>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::font::face {

class Face;

};  // namespace v3d::font::face

namespace v3d::font::text::internal {

/**
 * The faces' measures at the sizes one layout meets, each read once.
 **/
class MetricsCache {
 public:
    /**
     * @return a face's measures at a size in pixels and at normalized coordinates
     **/
    const face::Metrics& of(const boost::shared_ptr<face::Face>& face, float size, const std::vector<std::int32_t>& coordinates);

 private:
    std::map<std::tuple<const face::Face*, float, std::vector<std::int32_t>>, face::Metrics> metrics_;
};

};  // namespace v3d::font::text::internal
