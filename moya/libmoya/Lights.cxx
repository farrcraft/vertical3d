/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Lights.h"

#include <api/render/offline/Area.h>
#include <api/render/offline/sl/Placed.h>
#include <api/render/offline/trace/Primitive.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/mat4x4.hpp>

namespace v3d::moya {

void Lights::add(const Light & light) {
    lights_.push_back(light);
}

std::size_t Lights::size() const {
    return lights_.size();
}

const boost::shared_ptr<v3d::render::offline::Area> & Lights::area(unsigned int samples) {
    Light & last = lights_.back();
    last.area = boost::make_shared<v3d::render::offline::Area>(last.handle);
    last.area->samples(samples);
    return last.area;
}

std::vector<v3d::render::offline::sl::Placed> Lights::shining(const std::vector<std::string> & lit) const {
    std::vector<v3d::render::offline::sl::Placed> on;
    for (const Light & light : lights_) {
        if (std::find(lit.begin(), lit.end(), light.handle) == lit.end()) {
            continue;
        }
        v3d::render::offline::sl::Placed shining;
        shining.shader = light.shader;
        shining.placement = light.placement;
        shining.area = light.area;
        on.push_back(shining);
    }
    return on;
}

const v3d::render::offline::trace::Lights & Lights::traced(const std::vector<v3d::render::offline::sl::Placed> & on,
    const std::vector<std::string> & lit, const glm::mat4x4 & toWorld) {
    if (traced_ && tracedFor_ == lit && tracedCount_ == lights_.size()) {
        return traced_;
    }
    auto placed = boost::make_shared<std::vector<v3d::render::offline::sl::Placed> >();
    for (const v3d::render::offline::sl::Placed & light : on) {
        v3d::render::offline::sl::Placed world = light;
        world.placement = toWorld * light.placement;
        placed->push_back(world);
    }
    traced_ = placed;
    tracedFor_ = lit;
    tracedCount_ = lights_.size();
    return traced_;
}

};  // namespace v3d::moya
