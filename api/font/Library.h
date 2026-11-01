/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <boost/shared_ptr.hpp>

namespace v3d::font::internal {
class LibraryHandle;
};  // namespace v3d::font::internal

namespace v3d::font {

/**
 * The FreeType library every face of an app is opened through.
 *
 * A face holds its library, so the library is closed after the last face opened through it. A
 * library and its faces are used from one thread.
 **/
class Library final {
 public:
    Library();

    /**
     * @return whether FreeType was initialized. A face opened through a library that was not
     *         does not open
     **/
    bool valid() const;

    /**
     * @return the FreeType library, for the library's own sources
     **/
    internal::LibraryHandle& handle() const;

 private:
    boost::shared_ptr<internal::LibraryHandle> handle_;
};

};  // namespace v3d::font
