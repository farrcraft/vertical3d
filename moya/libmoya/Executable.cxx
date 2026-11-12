/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/Executable.h>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#else
#include <limits.h>
#include <unistd.h>
#endif

#include <cstddef>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/path.hpp>

namespace v3d::moya {

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

};  // namespace v3d::moya
