/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <MaterialXCore/Document.h>

#include <string>

namespace v3d::moya::material {

class Library;

/**
 * A MaterialX document read from a file, whose node definitions are the library's, so each of
 * its nodes resolves to the definition a material compiled from it would.
 *
 * @throws MaterialX::Exception when the file cannot be read or is not a MaterialX document
 **/
MaterialX::DocumentPtr readDocument(const Library & library, const std::string & file);

};  // namespace v3d::moya::material
