/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/device/CpuLibrary.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#include <intrin.h>
#include <immintrin.h>
#else
#include <dlfcn.h>
#include <limits.h>
#include <unistd.h>
#endif

#include <moya/libmoya/device/cpu/Isa.h>
#include <moya/libmoya/device/cpu/Module.h>

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

namespace v3d::moya::device {

namespace {

#ifdef _WIN32

/** The leaves of cpuid that report the processor's features and its extended features. **/
constexpr int kFeatures = 1;
constexpr int kExtended = 7;

/** The feature bits: of ECX from the first leaf, and of EBX from the extended one. **/
constexpr int kFma = 12;
constexpr int kOsxsave = 27;
constexpr int kAvx = 28;
constexpr int kBmi1 = 3;
constexpr int kAvx2 = 5;
constexpr int kBmi2 = 8;
constexpr int kAvx512F = 16;
constexpr int kAvx512Dq = 17;
constexpr int kAvx512Bw = 30;
constexpr int kAvx512Vl = 31;

/** The state XCR0 says the operating system saves: SSE and AVX, then the AVX-512 registers too. **/
constexpr std::uint64_t kYmmState = 0x6U;
constexpr std::uint64_t kZmmState = 0xe6U;

/** A bit of a register cpuid gives. **/
bool has(int value, int bit) {
    return (static_cast<unsigned int>(value) & (1U << static_cast<unsigned int>(bit))) != 0;
}

#endif

/** The directory of the running executable. **/
boost::filesystem::path executableDirectory() {
#ifdef _WIN32
    std::vector<wchar_t> buffer(MAX_PATH);
    for (;;) {
        const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
        if (length == 0) {
            throw std::runtime_error("the running executable's path cannot be found");
        }
        if (length < buffer.size()) {
            return boost::filesystem::path(std::wstring(buffer.data(), length)).parent_path();
        }
        buffer.resize(buffer.size() * 2);
    }
#else
    std::vector<char> buffer(PATH_MAX);
    const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size());
    if (length <= 0 || static_cast<std::size_t>(length) >= buffer.size()) {
        throw std::runtime_error("the running executable's path cannot be found");
    }
    return boost::filesystem::path(std::string(buffer.data(), static_cast<std::size_t>(length))).parent_path();
#endif
}

boost::filesystem::path file(cpu::Isa isa) {
#ifdef _WIN32
    const char * extension = ".dll";
#else
    const char * extension = ".so";
#endif
    return executableDirectory() / ("moya_cpu_" + isaName(isa) + extension);
}

};  // namespace

#ifdef _WIN32

cpu::Isa newestIsa() {
    int leaf1[4] = {};
    __cpuid(leaf1, kFeatures);
    int leaf7[4] = {};
    __cpuidex(leaf7, kExtended, 0);
    const int ecx = leaf1[2];
    const int ebx = leaf7[1];
    // the operating system saves the vector registers only where XGETBV says so
    const std::uint64_t xcr0 = has(ecx, kOsxsave) ? _xgetbv(0) : 0;
    const bool ymm = (xcr0 & kYmmState) == kYmmState;
    const bool zmm = (xcr0 & kZmmState) == kZmmState;
    const bool avx2 = ymm && has(ecx, kAvx) && has(ecx, kFma) && has(ebx, kAvx2) && has(ebx, kBmi1) && has(ebx, kBmi2);
    const bool avx512 = avx2 && zmm && has(ebx, kAvx512F) && has(ebx, kAvx512Dq) && has(ebx, kAvx512Bw) && has(ebx, kAvx512Vl);
    if (avx512) {
        return cpu::Isa::Avx512;
    }
    return avx2 ? cpu::Isa::Avx2 : cpu::Isa::Sse42;
}

#else

cpu::Isa newestIsa() {
    // GCC's and Clang's checks ask the operating system whether it saves the registers, too
    __builtin_cpu_init();
    if (__builtin_cpu_supports("avx512f") && __builtin_cpu_supports("avx512dq") && __builtin_cpu_supports("avx512bw") &&
        __builtin_cpu_supports("avx512vl")) {
        return cpu::Isa::Avx512;
    }
    if (__builtin_cpu_supports("avx2") && __builtin_cpu_supports("fma") && __builtin_cpu_supports("bmi") &&
        __builtin_cpu_supports("bmi2")) {
        return cpu::Isa::Avx2;
    }
    return cpu::Isa::Sse42;
}

#endif

std::string isaName(cpu::Isa isa) {
    switch (isa) {
        case cpu::Isa::Sse42:
            return "sse42";
        case cpu::Isa::Avx2:
            return "avx2";
        case cpu::Isa::Avx512:
            return "avx512";
    }
    return "sse42";
}

CpuLibrary::CpuLibrary(cpu::Isa isa) {
    const boost::filesystem::path at = file(isa);
#ifdef _WIN32
    handle_ = LoadLibraryW(at.c_str());
#else
    handle_ = dlopen(at.c_str(), RTLD_NOW | RTLD_LOCAL);
#endif
    if (handle_ == nullptr) {
        throw std::runtime_error("the CPU library " + at.string() + " cannot be loaded");
    }
#ifdef _WIN32
    FARPROC found = GetProcAddress(static_cast<HMODULE>(handle_), cpu::kModuleEntry);
    // a function's address converts to another function's type, which the table's entry is
    const auto entry = reinterpret_cast<cpu::ModuleEntry>(reinterpret_cast<void (*)()>(found));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
#else
    // POSIX defines the conversion of the object pointer dlsym returns to a function's
    const auto entry = reinterpret_cast<cpu::ModuleEntry>(dlsym(handle_, cpu::kModuleEntry));  // NOLINT(cppcoreguidelines-pro-type-reinterpret-cast)
#endif
    const cpu::Module * module = entry == nullptr ? nullptr : entry();
    if (module == nullptr || module->version != cpu::kModuleVersion || module->isa != isa) {
#ifdef _WIN32
        FreeLibrary(static_cast<HMODULE>(handle_));
#else
        dlclose(handle_);
#endif
        throw std::runtime_error("the CPU library " + at.string() + " is not the " + isaName(isa) + " library of this build");
    }
    module_ = module;
}

CpuLibrary::~CpuLibrary() {
#ifdef _WIN32
    FreeLibrary(static_cast<HMODULE>(handle_));
#else
    dlclose(handle_);
#endif
}

const cpu::Module & CpuLibrary::module() const {
    return *module_;
}

std::string CpuLibrary::path(cpu::Isa isa) {
    return file(isa).string();
}

};  // namespace v3d::moya::device
