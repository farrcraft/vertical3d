/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Launch.h"

#include <algorithm>
#include <charconv>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <system_error>

#include <boost/filesystem/path.hpp>

namespace v3d::engine {

/**
 **/
Launch Launch::parse(int argc, const char* const* argv) {
    Launch launch;
    if (argc > 0 && argv[0] != nullptr) {
        launch.program = boost::filesystem::path(argv[0]).stem().string();
    }
    for (int index = 1; index < argc; index++) {
        const std::string_view argument = argv[index];
        const bool last = index + 1 >= argc;
        if (argument == "--version") {
            launch.version = true;
        } else if (argument == "--capture") {
            if (last || std::string_view(argv[index + 1]).empty()) {
                throw std::invalid_argument("--capture needs the file to write");
            }
            launch.capture = argv[++index];
        } else if (argument == "--capture-after") {
            const std::string_view value = last ? std::string_view() : std::string_view(argv[index + 1]);
            unsigned int frame = 0;
            const auto [end, error] = std::from_chars(value.data(), value.data() + value.size(), frame);
            if (value.empty() || error != std::errc() || end != value.data() + value.size() || frame == 0) {
                throw std::invalid_argument("--capture-after needs a whole number of frames from 1 up");
            }
            launch.captureAfter = frame;
            index++;
        } else {
            launch.rest.emplace_back(argument);
        }
    }
    return launch;
}

/**
 **/
bool Launch::due(unsigned int frame) const noexcept {
    return !capture.empty() && frame == captureAfter;
}

/**
 **/
std::optional<std::string> Launch::option(std::string_view name) const {
    const auto found = std::ranges::find(rest, name);
    if (found == rest.end() || found + 1 == rest.end()) {
        return std::nullopt;
    }
    return *(found + 1);
}

};  // namespace v3d::engine
