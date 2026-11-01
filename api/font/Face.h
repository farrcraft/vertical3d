/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::font::internal {
class FaceHandle;
};  // namespace v3d::font::internal

namespace v3d::font {

class Library;

/**
 * One font file, held open from open() until the last holder lets it go.
 *
 * A face names its family and style as the file does. Its weight is the file's OS/2 weight
 * class, from 100 for thin to 900 for black, with 400 regular and 700 bold. A file with no OS/2
 * table is 700 when it says it is bold, and 400 otherwise.
 *
 * A glyph is named by its index in the face. Index 0 is the face's missing glyph.
 **/
class Face final {
 public:
    /**
     * Open a font file through a library.
     * @return the face, or null when the file does not open or has no Unicode character map,
     *         which is logged
     **/
    static boost::shared_ptr<Face> open(const boost::shared_ptr<Library>& library, const std::string& filename,
        const boost::shared_ptr<v3d::log::Logger>& logger);

    const std::string& filename() const noexcept;
    const std::string& family() const noexcept;
    const std::string& style() const noexcept;
    unsigned int weight() const noexcept;
    bool italic() const noexcept;

    /**
     * @return the index of a code point's glyph, or 0 when the face has none
     **/
    unsigned int glyphIndex(char32_t charcode) const;

    /**
     * @return whether the face has a glyph for a code point
     **/
    bool has(char32_t charcode) const;

    /**
     * @return the library the face was opened through
     **/
    const boost::shared_ptr<Library>& library() const noexcept;

    /**
     * @return the FreeType face, for the library's own sources
     **/
    internal::FaceHandle& handle() const;

 private:
    Face() = default;

    // declared before the face, so the face is closed before its library
    boost::shared_ptr<Library> library_;
    boost::shared_ptr<internal::FaceHandle> handle_;
    std::string filename_;
    std::string family_;
    std::string style_;
    unsigned int weight_ = 0;
    bool italic_ = false;
};

};  // namespace v3d::font
