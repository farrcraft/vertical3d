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
 * built out of them - see ADR-0004. The tag parameter keeps the handle types distinct,
 * so a texture handle cannot be passed where a pipeline handle is wanted.
 **/
template <typename Tag>
class Handle final {
 public:
    /**
     * An unset handle, referring to nothing.
     **/
    Handle() noexcept : id_(invalid) {
    }

    /**
     * @param id the registry slot the handle refers to
     **/
    explicit Handle(uint32_t id) noexcept : id_(id) {
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
     **/
    bool operator==(const Handle& other) const noexcept {
        return id_ == other.id_;
    }

    /**
     **/
    bool operator!=(const Handle& other) const noexcept {
        return id_ != other.id_;
    }

    /**
     **/
    bool operator<(const Handle& other) const noexcept {
        return id_ < other.id_;
    }

 private:
    static const uint32_t invalid = 0xFFFFFFFFu;

    uint32_t id_;
};

using PipelineHandle = Handle<struct PipelineTag>;
using MaterialHandle = Handle<struct MaterialTag>;
using TextureHandle = Handle<struct TextureTag>;

};  // namespace v3d::render::realtime
