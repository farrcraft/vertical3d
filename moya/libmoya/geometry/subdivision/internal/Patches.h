/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <opensubdiv/far/patchMap.h>
#include <opensubdiv/far/patchTable.h>
#include <opensubdiv/far/topologyRefiner.h>

#include <utility>

#include <boost/shared_ptr.hpp>

namespace v3d::moya::geometry::subdivision::internal {

/**
 * A topology refined adaptively by OpenSubdiv, and the table of patches over its limit surface,
 * behind geometry::Surface. The map finds the patch at a position on a patch face.
 **/
class Patches final {
 public:
    Patches(boost::shared_ptr<const OpenSubdiv::Far::TopologyRefiner> refiner, boost::shared_ptr<const OpenSubdiv::Far::PatchTable> table) :
        refiner_(std::move(refiner)), table_(std::move(table)), map_(*table_) {
    }

    const OpenSubdiv::Far::TopologyRefiner & refiner() const {
        return *refiner_;
    }

    const OpenSubdiv::Far::PatchTable & table() const {
        return *table_;
    }

    const OpenSubdiv::Far::PatchMap & map() const {
        return map_;
    }

 private:
    boost::shared_ptr<const OpenSubdiv::Far::TopologyRefiner> refiner_;
    boost::shared_ptr<const OpenSubdiv::Far::PatchTable> table_;
    OpenSubdiv::Far::PatchMap map_;
};

};  // namespace v3d::moya::geometry::subdivision::internal
