/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <functional>
#include <string>
#include <string_view>

namespace v3d::ui::input {

/**
 * The platform's clipboard, as a pair of callbacks. This library cannot reach the clipboard
 * without depending on SDL, so the app supplies it, as it supplies text measuring.
 *
 * An editor given neither callback still handles every other edit. A cut with no write
 * callback leaves the text in place, so the text is never lost. A paste with no read callback
 * inserts nothing.
 **/
struct Clipboard final {
    std::function<std::string()> read;
    std::function<void(std::string_view)> write;
};

};  // namespace v3d::ui::input
