/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/font/text/Bidi.h>
#include <api/font/text/Segmenter.h>
#include <api/font/text/style/Range.h>

#include <cstddef>
#include <string>
#include <vector>

#include "Run.h"

namespace v3d::font::text::shaping {

/**
 * Everything about a text that does not depend on the width it is laid out in: its levels, its
 * shaped runs, and where a caret may stop and a line may break.
 **/
struct Text final {
    std::string text;
    Bidi bidi;
    std::vector<style::Range> ranges{};    /**< the styles it was shaped in, adjacent ones that shape alike joined **/
    std::vector<Run> runs{};        /**< in the text's order **/
    std::vector<std::size_t> runRanges{}; /**< for each run, the index of the range it was shaped in **/
    std::vector<std::size_t> graphemes{}; /**< as Segmenter::graphemes() gives them **/
    std::vector<Segmenter::LineBreak> breaks{};      /**< as Segmenter::lineBreaks() gives them **/
};

};  // namespace v3d::font::text::shaping
