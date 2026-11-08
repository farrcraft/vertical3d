/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <cstddef>
#include <string>
#include <string_view>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::moya::colour {

namespace internal {
class Processor;
class Configuration;
};  // namespace internal

/**
 * A conversion between colour spaces that an OpenColorIO configuration gives. It is safe to apply
 * from any number of threads at once.
 **/
class Transform final {
 public:
    explicit Transform(boost::shared_ptr<const internal::Processor> processor);

    /** One colour, converted. **/
    glm::vec3 apply(const glm::vec3 & rgb) const;

    /**
     * Converts the first three floats of each of a run of pixels in place, leaving the rest.
     *
     * @param values the first pixel's first float
     * @param pixels how many pixels
     * @param stride the floats from one pixel's start to the next's, at least 3
     **/
    void apply(float * values, std::size_t pixels, std::size_t stride) const;

 private:
    boost::shared_ptr<const internal::Processor> processor_;
};

/**
 * An OpenColorIO configuration: the colour spaces colours are named in, the working space, and the
 * displays a picture is viewed on.
 *
 * The film holds the beauty in ACEScg, and RGB-to-spectrum lookups take ACEScg, whatever the
 * working space. The transforms here reach ACEScg through the configuration's interchange roles,
 * aces_interchange or cie_xyz_d65_interchange, which OpenColorIO's ACES configurations have.
 **/
class Config final {
 public:
    /** The configuration moya uses when given none: OpenColorIO's built-in ACES 2.0 CG one. **/
    static constexpr std::string_view kBuiltIn = "ocio://cg-config-v4.0.0_aces-v2.0_ocio-v2.5";

    /** The built-in configuration. **/
    Config();

    /**
     * A configuration from a file, or a built-in one by its ocio:// name.
     *
     * @param path the file, as UTF-8
     * @throws std::runtime_error, from OpenColorIO, when it cannot be read or is not valid
     **/
    explicit Config(const std::string & path);

    /** The configuration's name. **/
    std::string name() const;

    /** The working space: the colour space of the configuration's scene_linear role. **/
    std::string workingSpace() const;

    /** The working space's Color Interop Forum ID, such as lin_ap1_scene, or empty for none. **/
    std::string workingInteropId() const;

    /**
     * Converts a colour named in a space into ACEScg, for the spectrum table. A colour that names
     * no space is in the working space.
     *
     * @param space a colour space's name or role, or empty for the working space
     * @throws std::runtime_error, from OpenColorIO, when the configuration has no such space, or no
     *         interchange role to reach ACEScg through
     **/
    Transform input(const std::string & space) const;

    /** Converts the film's ACEScg into the working space, which outputs are written in. **/
    Transform output() const;

    /** The display a picture is shown on when none is named. **/
    std::string defaultDisplay() const;

    /** The view of a display used when none is named. **/
    std::string defaultView(const std::string & display) const;

    /**
     * Converts the film's ACEScg to what a display shows through one of its views, such as the sRGB
     * display through ACES 2.0's SDR view, for an 8-bit picture or the editor's render view.
     *
     * @throws std::runtime_error, from OpenColorIO, when the configuration lacks either
     **/
    Transform display(const std::string & display, const std::string & view) const;

 private:
    boost::shared_ptr<const internal::Configuration> config_;
};

};  // namespace v3d::moya::colour
