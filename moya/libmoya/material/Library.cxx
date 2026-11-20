/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include <moya/libmoya/material/Library.h>

#include <moya/libmoya/Executable.h>
#include <moya/libmoya/material/internal/Definitions.h>
#include <moya/libmoya/material/internal/HairGraphs.h>
#include <moya/libmoya/material/internal/TranslationGraphs.h>
#include <MaterialXCore/Definition.h>
#include <MaterialXCore/Document.h>
#include <MaterialXCore/Exception.h>
#include <MaterialXCore/Node.h>
#include <MaterialXFormat/File.h>
#include <MaterialXFormat/Util.h>
#include <MaterialXFormat/XmlIo.h>

#include <algorithm>
#include <stdexcept>
#include <string>
#include <vector>

#include <boost/filesystem/directory.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/make_shared.hpp>

namespace v3d::moya::material {

namespace {

namespace mx = MaterialX;

/** Every .mtlx file under a directory, in a fixed order so that every run reads them alike. **/
std::vector<boost::filesystem::path> documentsUnder(const boost::filesystem::path & directory) {
    if (!boost::filesystem::is_directory(directory)) {
        throw std::runtime_error("MaterialX's standard libraries are not in " + directory.string());
    }
    std::vector<boost::filesystem::path> files;
    for (const boost::filesystem::directory_entry & entry : boost::filesystem::recursive_directory_iterator(directory)) {
        if (entry.status().type() == boost::filesystem::regular_file && entry.path().extension() == ".mtlx") {
            files.push_back(entry.path());
        }
    }
    std::sort(files.begin(), files.end());
    return files;
}

mx::NodeDefPtr nodeDef(const internal::Definitions & definitions, const std::string & name) {
    return definitions.get()->getNodeDef(name);
}

};  // namespace

Library::Library() : Library(executableDirectory() / kDirectory) {
}

Library::Library(const boost::filesystem::path & directory) {
    const mx::DocumentPtr document = mx::createDocument();
    const mx::FileSearchPath searchPath(mx::FilePath(directory.string()));
    for (const boost::filesystem::path & file : documentsUnder(directory)) {
        try {
            mx::loadLibrary(mx::FilePath(file.string()), document, searchPath);
        } catch (const mx::Exception & error) {
            throw std::runtime_error(file.string() + " is not a valid MaterialX document: " + error.what());
        }
    }
    // moya's own translations, for the surface shaders MaterialX ships none of onto OpenPBR
    const mx::DocumentPtr translations = mx::createDocument();
    mx::readFromXmlString(translations, internal::translationGraphs());
    document->importLibrary(translations);
    // moya's graphs of the nodes that give a hair's parameters, which MaterialX implements only
    // in its shader generators
    const mx::DocumentPtr hair = mx::createDocument();
    mx::readFromXmlString(hair, internal::hairGraphs());
    document->importLibrary(hair);
    definitions_ = boost::make_shared<const internal::Definitions>(document);
}

const Library & Library::installed() {
    // a failed read leaves nothing behind, so the next call reads again and throws again
    static const Library library;
    return library;
}

bool Library::defines(const std::string & definition) const {
    return nodeDef(*definitions_, definition) != nullptr;
}

std::string Library::version(const std::string & definition) const {
    const mx::NodeDefPtr def = nodeDef(*definitions_, definition);
    return def ? def->getVersionString() : std::string();
}

std::string Library::graph(const std::string & definition) const {
    const mx::NodeDefPtr def = nodeDef(*definitions_, definition);
    if (!def) {
        return {};
    }
    // MaterialX resolves an implementation element that names a graph to the graph itself, so
    // anything else is implemented in code
    const mx::InterfaceElementPtr implementation = def->getImplementation();
    return implementation && implementation->isA<mx::NodeGraph>() ? implementation->getName() : std::string();
}

};  // namespace v3d::moya::material
