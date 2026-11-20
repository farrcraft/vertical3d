/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <OpenColorIO/OpenColorIO.h>

#include <utility>

namespace v3d::moya::colour::internal {

/** An OpenColorIO processor on the CPU, at 32-bit floats, behind colour::Transform. **/
class Processor final {
 public:
    explicit Processor(OCIO_NAMESPACE::ConstCPUProcessorRcPtr cpu) : cpu_(std::move(cpu)) {
    }

    const OCIO_NAMESPACE::ConstCPUProcessorRcPtr & cpu() const {
        return cpu_;
    }

 private:
    OCIO_NAMESPACE::ConstCPUProcessorRcPtr cpu_;
};

};  // namespace v3d::moya::colour::internal
