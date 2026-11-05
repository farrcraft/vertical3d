/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "SvgDocuments.h"

#include <api/font/internal/SvgReader.h>
#include <thorvg.h>

#include <cstddef>
#include <utility>

namespace v3d::font::internal {

tvg::Picture* SvgDocuments::picture(const SvgDocument& document) {
    const auto key = std::make_pair(document.text.data(), document.text.size());
    const auto found = pictures_.find(key);
    if (found != pictures_.end()) {
        return found->second.get();
    }
    parsed_++;
    return pictures_.emplace(key, SvgReader::load(document)).first->second.get();
}

std::size_t SvgDocuments::parsed() const noexcept {
    return parsed_;
}

};  // namespace v3d::font::internal
