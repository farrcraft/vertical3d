/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <unicode/ubrk.h>

#include <string>

namespace v3d::font::internal {

/**
 * Owns ICU's break iterators for clusters, words and lines in one locale, from construction to
 * destruction. Each is null when its rules could not be loaded.
 **/
class BreakHandle final {
 public:
    explicit BreakHandle(const std::string& locale);
    ~BreakHandle();
    BreakHandle(const BreakHandle&) = delete;
    BreakHandle& operator=(const BreakHandle&) = delete;

    UBreakIterator* graphemes = nullptr;
    UBreakIterator* words = nullptr;
    UBreakIterator* lines = nullptr;
};

};  // namespace v3d::font::internal
