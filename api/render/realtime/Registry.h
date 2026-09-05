/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <utility>
#include <vector>

#include "Handle.h"

namespace v3d::render::realtime {

/**
 * The resources of one kind the engine owns, each addressed by a handle.
 *
 * Draw items refer to pipelines, materials and textures by handle so that a sort key can
 * be built from them - a pointer sorts by whatever the allocator happened to hand out,
 * which reorders a frame differently every run. A registry slot is stable for the life of
 * the registry, and its index is the order things sort in.
 *
 * Slots are never reused while the registry lives, so a handle cannot come to refer to
 * something other than what it was given for.
 **/
template <typename Tag, typename Resource>
class Registry final {
 public:
    /**
     * Take ownership of a resource.
     * @return the handle it is addressed by from now on
     **/
    Handle<Tag> add(const Resource& resource) {
        resources_.push_back(resource);
        return Handle<Tag>(static_cast<uint32_t>(resources_.size() - 1));
    }

    /**
     * @return the resource the handle refers to, or nullptr if it refers to nothing
     **/
    const Resource* resolve(const Handle<Tag>& handle) const {
        if (!handle.valid() || handle.id() >= resources_.size()) {
            return nullptr;
        }
        return &resources_[handle.id()];
    }

    /**
     * @return the resource the handle refers to, or nullptr if it refers to nothing
     **/
    Resource* resolve(const Handle<Tag>& handle) {
        if (!handle.valid() || handle.id() >= resources_.size()) {
            return nullptr;
        }
        return &resources_[handle.id()];
    }

    /**
     * @return how many resources the registry holds
     **/
    std::size_t count() const noexcept {
        return resources_.size();
    }

    /**
     * @return every resource, in handle order, for a caller that has to destroy them
     **/
    const std::vector<Resource>& resources() const noexcept {
        return resources_;
    }

    /**
     * Forget everything. Every handle handed out before this refers to nothing after it,
     * so a caller that owns device resources has to destroy them first.
     **/
    void clear() noexcept {
        resources_.clear();
    }

 private:
    std::vector<Resource> resources_;
};

};  // namespace v3d::render::realtime
