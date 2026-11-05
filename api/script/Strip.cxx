/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Strip.h"

#include <api/script/internal/Eraser.h>

#include <algorithm>
#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace v3d::script {

/**
 **/
Stripped strip(std::string_view source) {
    internal::Eraser eraser(source);
    std::string code = eraser.run();
    if (!eraser.why().empty()) {
        const std::size_t where = std::min(eraser.where(), source.size());
        const std::string_view before = source.substr(0, where);
        const std::size_t line = static_cast<std::size_t>(std::count(before.begin(), before.end(), '\n')) + 1;
        const std::size_t start = before.rfind('\n');
        const std::size_t column = where - (start == std::string_view::npos ? 0 : start + 1) + 1;
        return { .code = {}, .problem = Problem{ .line = line, .column = column, .message = eraser.why() } };
    }
    return { .code = std::move(code), .problem = std::nullopt };
}

};  // namespace v3d::script
