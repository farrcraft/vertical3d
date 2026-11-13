/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/texture/SearchPath.h>

#include <cctype>
#include <optional>
#include <string>
#include <vector>

#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

namespace v3d::moya::texture {

std::vector<std::string> directoriesOf(const std::string & path) {
    std::vector<std::string> out;
    std::string current;
    for (const char letter : path) {
        const bool drive = letter == ':' && current.size() == 1U && std::isalpha(static_cast<unsigned char>(current.front())) != 0;
        if ((letter == ':' && !drive) || letter == ';') {
            if (!current.empty()) {
                out.push_back(current);
            }
            current.clear();
            continue;
        }
        current += letter;
    }
    if (!current.empty()) {
        out.push_back(current);
    }
    return out;
}

std::optional<std::string> found(const std::string & name, const std::vector<std::string> & directories) {
    const boost::filesystem::path given(name);
    boost::system::error_code ignored;
    if (given.is_absolute() || directories.empty()) {
        if (boost::filesystem::is_regular_file(given, ignored)) {
            return name;
        }
        return std::nullopt;
    }
    for (const std::string & directory : directories) {
        const boost::filesystem::path candidate = boost::filesystem::path(directory) / given;
        if (boost::filesystem::is_regular_file(candidate, ignored)) {
            return candidate.string();
        }
    }
    return std::nullopt;
}

};  // namespace v3d::moya::texture
