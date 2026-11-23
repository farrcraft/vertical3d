/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/texture/Address.h>
#include <moya/libmoya/kernel/texture/Filtering.h>

namespace v3d::moya::kernel::texture {

/** How a lookup reads: its address mode along u and along v, and how it filters. **/
struct Sampling {
    Address s { Address::Periodic };
    Address t { Address::Periodic };
    Filtering filtering { Filtering::Linear };
};

};  // namespace v3d::moya::kernel::texture
