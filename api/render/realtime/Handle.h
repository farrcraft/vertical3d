/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstdint>

namespace v3d::render::realtime {

/**
 * An opaque, comparable reference to a resource the engine owns.
 *
 * Draw items name resources by handle rather than by pointer so that a sort key can be
 * built out of them. The tag parameter keeps the handle types distinct,
 * so a texture handle cannot be passed where a pipeline handle is wanted.
 *
 * A slot can be reused once what was in it is released, so a handle also carries the
 * generation of the occupant it was given for. Two handles to the same slot with
 * different generations are different handles; only the slot is a sort order.
 **/
template <typename Tag>
class Handle final {
 public:
    /**
     * An unset handle, referring to nothing.
     **/
    Handle() noexcept : id_(invalid), generation_(0) {
    }

    /**
     * @param id the registry slot the handle refers to
     * @param generation which occupant of that slot it refers to
     **/
    explicit Handle(uint32_t id, uint32_t generation = 0) noexcept : id_(id), generation_(generation) {
    }

    /**
     * @return whether the handle refers to anything
     **/
    bool valid() const noexcept {
        return id_ != invalid;
    }

    /**
     * @return the registry slot, which is also the handle's sort order
     **/
    uint32_t id() const noexcept {
        return id_;
    }

    /**
     * @return which occupant of the slot the handle refers to
     **/
    uint32_t generation() const noexcept {
        return generation_;
    }

    /**
     **/
    bool operator==(const Handle& other) const noexcept {
        return id_ == other.id_ && generation_ == other.generation_;
    }

    /**
     **/
    bool operator!=(const Handle& other) const noexcept {
        return !(*this == other);
    }

    /**
     **/
    bool operator<(const Handle& other) const noexcept {
        return id_ < other.id_ || (id_ == other.id_ && generation_ < other.generation_);
    }

 private:
    static const uint32_t invalid = 0xFFFFFFFFu;

    uint32_t id_;
    uint32_t generation_;
};

using PipelineHandle = Handle<struct PipelineTag>;
using MaterialHandle = Handle<struct MaterialTag>;
using TextureHandle = Handle<struct TextureTag>;
using MeshHandle = Handle<struct MeshTag>;

};  // namespace v3d::render::realtime
