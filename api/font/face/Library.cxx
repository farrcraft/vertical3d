/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Library.h"

#include <api/font/internal/LibraryHandle.h>

#include <boost/make_shared.hpp>

namespace v3d::font::face {

Library::Library() :
    handle_(boost::make_shared<internal::LibraryHandle>()) {
}

bool Library::valid() const {
    return handle_->library != nullptr;
}

internal::LibraryHandle& Library::handle() const {
    return *handle_;
}

};  // namespace v3d::font::face
