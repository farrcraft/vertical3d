/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <stdexcept>
#include <string>

namespace v3d::script {

/**
 * What a function a script calls throws to refuse the call. The script sees it as an Error with
 * the same message, which it can catch.
 **/
class Refused final : public std::runtime_error {
 public:
    explicit Refused(const std::string& message);
};

};  // namespace v3d::script
