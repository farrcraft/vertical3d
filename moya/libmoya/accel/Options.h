/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

namespace v3d::moya::accel {

/** How a hierarchy is built. **/
struct Options {
    /** Whether a primitive may be split in space, so that it is referenced from two leaves. **/
    bool spatial { true };

    /**
     * How many references spatial splits may add, as a fraction of the primitives. Each node
     * shares its own part among its children by their counts, so the budget does not depend on
     * which thread builds which node.
     **/
    float budget { 1.0F };

    /** Whether a hierarchy over moving primitives may split a node by time. **/
    bool temporal { true };
};

};  // namespace v3d::moya::accel
