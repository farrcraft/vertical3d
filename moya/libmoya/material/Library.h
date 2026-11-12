/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <string>

#include <boost/filesystem/path.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::moya::material {

namespace internal {
class Definitions;
};  // namespace internal

/**
 * MaterialX's standard libraries: the node definitions that a material's nodes name, and the node
 * graphs that implement the definitions built from other nodes.
 *
 * The libraries are installed in a directory beside moya, holding MaterialX's own folders such as
 * stdlib, pbrlib and bxdf. Every .mtlx file under it is read, so the definitions do not depend on
 * a MaterialX install or on MaterialX's search path in the environment.
 **/
class Library final {
 public:
    /** The directory beside the running executable that holds the libraries. **/
    static constexpr const char * kDirectory = "materialx";

    /**
     * The libraries installed beside the running executable.
     *
     * @throws std::runtime_error when the directory is missing or a file in it is not valid
     **/
    Library();

    /**
     * The libraries in a directory.
     *
     * @throws std::runtime_error when the directory is missing or a file in it is not valid
     **/
    explicit Library(const boost::filesystem::path & directory);

    /**
     * The libraries installed beside the running executable, read the first time they are asked
     * for and shared by every caller after.
     *
     * @throws std::runtime_error as Library() does, and again on each later call
     **/
    static const Library & installed();

    /** Whether a node definition is defined, such as ND_open_pbr_surface_surfaceshader. **/
    bool defines(const std::string & definition) const;

    /** A node definition's version, or empty when it gives none or is not defined. **/
    std::string version(const std::string & definition) const;

    /**
     * The name of the node graph that implements a node definition, or empty when the definition is
     * implemented in code or is not defined.
     **/
    std::string graph(const std::string & definition) const;

    /** The document MaterialX read the libraries into. **/
    const internal::Definitions & definitions() const {
        return *definitions_;
    }

 private:
    boost::shared_ptr<const internal::Definitions> definitions_;
};

};  // namespace v3d::moya::material
