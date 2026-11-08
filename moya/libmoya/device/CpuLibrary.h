/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/cpu/Module.h>

#include <string>

namespace v3d::moya::device {

/**
 * The newest instruction set this processor and its operating system run: AVX-512 with its
 * foundation, double and quad word, byte and word, and vector length extensions, or AVX2 with
 * FMA, BMI1 and BMI2, or SSE4.2.
 **/
cpu::Isa newestIsa();

/** The instruction set's name, as its library is named after it: sse42, avx2 or avx512. **/
std::string isaName(cpu::Isa isa);

/**
 * One of the CPU's libraries, loaded for as long as this object lives.
 *
 * A library is named moya_cpu_ and its instruction set's name, with the platform's extension for
 * a shared library, and is loaded from the directory of the running executable.
 **/
class CpuLibrary final {
 public:
    /**
     * @throws std::runtime_error when the library cannot be loaded, exports no table, or its table
     *         is of another version or names another instruction set
     **/
    explicit CpuLibrary(cpu::Isa isa);
    ~CpuLibrary();
    CpuLibrary(const CpuLibrary &) = delete;
    CpuLibrary & operator=(const CpuLibrary &) = delete;

    const cpu::Module & module() const;

    /** The file a library of an instruction set is loaded from, as UTF-8. **/
    static std::string path(cpu::Isa isa);

 private:
    void * handle_ { nullptr };
    const cpu::Module * module_ { nullptr };
};

};  // namespace v3d::moya::device
