/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/font/internal/SvgReader.h>
#include <thorvg.h>

#include <cstddef>
#include <map>
#include <memory>
#include <utility>

namespace v3d::font::internal {

/**
 * The SVG documents of one face, each parsed by ThorVG the first time a glyph of it is read and
 * kept while the face is open. A font may keep many glyphs in one document, which is then parsed
 * once. A document is found by where its text is, which stays put while the face is open. One
 * that does not parse is kept as none, so it is not tried again. It is used from one thread, as
 * its face is.
 **/
class SvgDocuments final {
 public:
    /**
     * @return a document's picture, parsed the first time it is asked for, or null when it does
     *         not parse
     **/
    tvg::Picture* picture(const SvgDocument& document);

    /**
     * @return how many documents have been parsed rather than found
     **/
    std::size_t parsed() const noexcept;

 private:
    std::map<std::pair<const char*, std::size_t>, std::unique_ptr<tvg::Picture, SvgReader::Release>> pictures_;
    std::size_t parsed_ = 0;
};

};  // namespace v3d::font::internal
