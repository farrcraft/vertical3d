/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <vector>

#include "Handle.h"

namespace v3d::render::realtime {

/**
 * The resources of one kind the engine owns, each addressed by a handle.
 *
 * Draw items refer to pipelines, materials and textures by handle so that a sort key can
 * be built from them - a pointer sorts by whatever the allocator happened to hand out,
 * which reorders a frame differently every run. A handle's slot is its sort order.
 *
 * A released slot is reused by a later add, with its generation moved on, and a handle
 * whose generation is not its slot's current one resolves to nothing. A handle can therefore
 * stop referring to anything, but it cannot come to refer to something other than what it
 * was given for.
 **/
template <typename Tag, typename Resource>
class Registry final {
 public:
    /**
     * Take ownership of a resource.
     * @return the handle it is addressed by until it is released
     **/
    Handle<Tag> add(const Resource& resource) {
        if (free_.empty()) {
            slots_.push_back(Slot{resource, 0});
            live_++;
            return Handle<Tag>(static_cast<uint32_t>(slots_.size() - 1), 0);
        }
        const uint32_t id = free_.back();
        free_.pop_back();
        Slot& slot = slots_[id];
        slot.resource = resource;
        live_++;
        return Handle<Tag>(id, slot.generation);
    }

    /**
     * Stop addressing a resource. The handle, and every copy of it, resolves to nothing from
     * now on, and its slot is free for the next add.
     *
     * @return what the handle referred to, for a caller that has to destroy it, or nothing
     *         if it referred to nothing - a handle released twice, say
     **/
    std::optional<Resource> release(const Handle<Tag>& handle) {
        Slot* slot = live(handle);
        if (slot == nullptr) {
            return std::nullopt;
        }
        std::optional<Resource> released = std::move(slot->resource);
        slot->resource.reset();
        slot->generation++;
        free_.push_back(handle.id());
        live_--;
        return released;
    }

    /**
     * @return the resource the handle refers to, or nullptr if it refers to nothing
     **/
    const Resource* resolve(const Handle<Tag>& handle) const {
        const Slot* slot = live(handle);
        return slot == nullptr ? nullptr : &*slot->resource;
    }

    /**
     * @return the resource the handle refers to, or nullptr if it refers to nothing
     **/
    Resource* resolve(const Handle<Tag>& handle) {
        Slot* slot = live(handle);
        return slot == nullptr ? nullptr : &*slot->resource;
    }

    /**
     * @return how many resources the registry holds, not counting released slots
     **/
    std::size_t count() const noexcept {
        return live_;
    }

    /**
     * Call a function with every resource still held, in slot order, for a caller that has
     * to destroy them.
     **/
    template <typename Function>
    void each(Function function) const {
        for (const Slot& slot : slots_) {
            if (slot.resource) {
                function(*slot.resource);
            }
        }
    }

    /**
     * Release everything. Every handle handed out before this refers to nothing after it,
     * so a caller that owns device resources has to destroy them first.
     **/
    void clear() {
        for (uint32_t id = 0; id < slots_.size(); id++) {
            release(Handle<Tag>(id, slots_[id].generation));
        }
    }

 private:
    struct Slot {
        std::optional<Resource> resource;
        uint32_t generation;
    };

    // the slot the handle names, if its occupant is still the one the handle was given for
    template <typename Slots>
    static auto find(Slots& slots, const Handle<Tag>& handle) -> decltype(&slots[0]) {
        if (!handle.valid() || handle.id() >= slots.size()) {
            return nullptr;
        }
        auto& slot = slots[handle.id()];
        if (!slot.resource || slot.generation != handle.generation()) {
            return nullptr;
        }
        return &slot;
    }

    const Slot* live(const Handle<Tag>& handle) const {
        return find(slots_, handle);
    }

    Slot* live(const Handle<Tag>& handle) {
        return find(slots_, handle);
    }

    std::vector<Slot> slots_;
    std::vector<uint32_t> free_;
    std::size_t live_ = 0;
};

};  // namespace v3d::render::realtime
