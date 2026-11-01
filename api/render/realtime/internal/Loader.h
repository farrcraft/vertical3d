/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

namespace v3d::render::realtime::internal {

/**
 * The Vulkan loader, loaded through SDL while this exists. SDL counts the loads, so a window
 * that loaded it as well keeps it.
 **/
class Loader final {
 public:
    Loader();
    ~Loader();
    Loader(const Loader&) = delete;
    Loader& operator=(const Loader&) = delete;
    Loader(Loader&&) = delete;
    Loader& operator=(Loader&&) = delete;
};

};  // namespace v3d::render::realtime::internal
