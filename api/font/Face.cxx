/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Face.h"

#include <api/font/internal/FaceHandle.h>
#include <api/font/internal/LibraryHandle.h>
#include <api/log/Logger.h>

#include "FaceStyle.h"
#include "Library.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::font {

namespace {

/**
 * The weight of a bold face, as the OS/2 weight class names it.
 **/
constexpr unsigned int BOLD = 700;

};  // namespace

boost::shared_ptr<Face> Face::open(const boost::shared_ptr<Library>& library, const std::string& filename,
    const boost::shared_ptr<v3d::log::Logger>& logger) {
    if (!library || !library->valid()) {
        logger->get()->error("{} could not be opened, because FreeType was not initialized", filename);
        return boost::shared_ptr<Face>();
    }
    boost::shared_ptr<Face> made(new Face());
    made->library_ = library;
    made->handle_ = boost::make_shared<internal::FaceHandle>(library->handle().library, filename.c_str());
    FT_Face face = made->handle_->face;
    if (face == nullptr) {
        logger->get()->error("{} could not be opened as a font with a Unicode character map", filename);
        return boost::shared_ptr<Face>();
    }
    made->filename_ = filename;
    made->family_ = face->family_name != nullptr ? face->family_name : "";
    made->style_ = face->style_name != nullptr ? face->style_name : "";
    made->italic_ = (face->style_flags & FT_STYLE_FLAG_ITALIC) != 0;
    const TT_OS2* os2 = static_cast<const TT_OS2*>(FT_Get_Sfnt_Table(face, FT_SFNT_OS2));
    if (os2 != nullptr && os2->usWeightClass > 0) {
        made->weight_ = os2->usWeightClass;
    } else {
        made->weight_ = (face->style_flags & FT_STYLE_FLAG_BOLD) != 0 ? BOLD : FaceStyle::REGULAR;
    }
    return made;
}

const std::string& Face::filename() const noexcept {
    return filename_;
}

const std::string& Face::family() const noexcept {
    return family_;
}

const std::string& Face::style() const noexcept {
    return style_;
}

unsigned int Face::weight() const noexcept {
    return weight_;
}

bool Face::italic() const noexcept {
    return italic_;
}

unsigned int Face::glyphIndex(char32_t charcode) const {
    return FT_Get_Char_Index(handle_->face, charcode);
}

bool Face::has(char32_t charcode) const {
    return glyphIndex(charcode) != 0;
}

const boost::shared_ptr<Library>& Face::library() const noexcept {
    return library_;
}

internal::FaceHandle& Face::handle() const {
    return *handle_;
}

};  // namespace v3d::font
