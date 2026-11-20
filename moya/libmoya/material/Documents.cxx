/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/Documents.h>

#include <moya/libmoya/material/internal/Definitions.h>
#include <moya/libmoya/material/Library.h>
#include <MaterialXCore/Document.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/XmlIo.h>

#include <string>

namespace v3d::moya::material {

MaterialX::DocumentPtr readDocument(const Library & library, const std::string & file) {
    const MaterialX::DocumentPtr out = MaterialX::createDocument();
    out->setDataLibrary(library.definitions().get());
    MaterialX::readFromXmlFile(out, MaterialX::FilePath(file));
    return out;
}

};  // namespace v3d::moya::material
