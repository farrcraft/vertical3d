/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "Pass.h"

#include <cstddef>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
/**
 * Everything to be drawn for one image, as a list of passes.
 *
 * A frame is built up during a tick and recorded in one step at the end of it. Compositing,
 * offscreen targets and an editor's several viewports are all extra passes over the same
 * frame, not a different kind of frame.
 **/
class Frame {
 public:
    /**
     * The pass of that name, added to the end of the list if the frame has none.
     * @return the pass, which stays valid until the frame is destroyed
     **/
    boost::shared_ptr<Pass> pass(const std::string& name);

    /**
     * @return the passes, in the order they were created
     **/
    const std::vector<boost::shared_ptr<Pass>>& passes() const noexcept;

    /**
     * The passes in the order they are recorded: every pass drawing into a target before
     * every pass that reads() it, and otherwise the order they were created in.
     * Passes drawing into one target, the swapchain image included, always keep the order
     * they were created in, since each draws over what the one before it left.
     *
     * @throw std::runtime_error if two passes each read what the other draws, which no order
     *        can record
     **/
    std::vector<boost::shared_ptr<Pass>> ordered() const;

    /**
     * The context's depth buffer serves only passes drawing into the swapchain image. A pass
     * with a target of its own attaches that target's depth.
     *
     * @return whether a pass drawing into the swapchain image tests depth
     **/
    bool swapchainDepth() const noexcept;

    /**
     * What ordered() places a pass by: the identity of what it draws into, null for the
     * swapchain image, and of what it reads. A pass reading what it also draws into is reading
     * that target's previous frame, and is ordered against the others drawing into it only by
     * when it was created.
     **/
    struct Node final {
        const void* writes = nullptr;
        std::vector<const void*> reads;
    };

    /**
     * ordered() over identities alone, so that the ordering can be tested without a device to
     * make a target on.
     *
     * @return the indices of the nodes in the order they are recorded
     * @throw std::runtime_error on a cycle
     **/
    static std::vector<std::size_t> order(const std::vector<Node>& nodes);

    /**
     * Drop what every pass has collected, keeping the passes themselves.
     **/
    void reset() noexcept;

 private:
    std::vector<boost::shared_ptr<Pass>> passes_;
};
};  // namespace v3d::render::realtime
