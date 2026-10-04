/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Frame.h"

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/make_shared.hpp>

namespace v3d::render::realtime {

/**
 **/
Frame::Frame(const boost::shared_ptr<Context>& context) :
    context_(context) {
}

/**
 **/
boost::shared_ptr<Pass> Frame::pass(const std::string& name) {
    for (const boost::shared_ptr<Pass>& pass : passes_) {
        if (pass->name() == name) {
            return pass;
        }
    }

    boost::shared_ptr<Pass> pass = boost::make_shared<Pass>(name);
    passes_.push_back(pass);
    return pass;
}

/**
 **/
const std::vector<boost::shared_ptr<Pass>>& Frame::passes() const noexcept {
    return passes_;
}

/**
 **/
std::vector<boost::shared_ptr<Pass>> Frame::ordered() const {
    std::vector<Node> nodes(passes_.size());
    for (std::size_t index = 0; index < passes_.size(); ++index) {
        nodes[index].writes = passes_[index]->target().get();
        for (const boost::shared_ptr<vulkan::frame::RenderTarget>& read : passes_[index]->reads()) {
            nodes[index].reads.push_back(read.get());
        }
    }

    std::vector<boost::shared_ptr<Pass>> ordered;
    ordered.reserve(passes_.size());
    for (const std::size_t index : order(nodes)) {
        ordered.push_back(passes_[index]);
    }
    return ordered;
}

/**
 **/
std::vector<std::size_t> Frame::order(const std::vector<Node>& nodes) {
    // whether the first has to be recorded before the second: both draw into one target and
    // the first was created first, or the first draws into something the second reads
    const auto before = [&nodes](std::size_t first, std::size_t second) {
        const void* target = nodes[first].writes;
        if (nodes[second].writes == target) {
            return first < second;
        }
        return target != nullptr &&
            std::find(nodes[second].reads.begin(), nodes[second].reads.end(), target) != nodes[second].reads.end();
    };

    // the earliest created pass whose writers are all placed goes next, so that passes the
    // reads do not order keep the order they were created in. A frame is a handful of passes,
    // so the quadratic walk is cheaper than building a graph for it
    std::vector<std::size_t> order;
    std::vector<bool> placed(nodes.size(), false);
    while (order.size() < nodes.size()) {
        std::size_t next = nodes.size();
        for (std::size_t candidate = 0; candidate < nodes.size() && next == nodes.size(); ++candidate) {
            if (placed[candidate]) {
                continue;
            }
            bool ready = true;
            for (std::size_t writer = 0; writer < nodes.size() && ready; ++writer) {
                ready = placed[writer] || !before(writer, candidate);
            }
            if (ready) {
                next = candidate;
            }
        }
        if (next == nodes.size()) {
            throw std::runtime_error("The frame's passes each read what another draws, so no order can record them");
        }
        placed[next] = true;
        order.push_back(next);
    }
    return order;
}

/**
 **/
boost::shared_ptr<Context> Frame::context() const noexcept {
    return context_;
}

/**
 **/
void Frame::reset() noexcept {
    for (const boost::shared_ptr<Pass>& pass : passes_) {
        pass->reset();
    }
}

};  // namespace v3d::render::realtime
