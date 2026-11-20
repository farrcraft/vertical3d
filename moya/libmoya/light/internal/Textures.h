/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/colour/Config.h>
#include <moya/libmoya/kernel/light/Image.h>

#include <cstdint>
#include <map>
#include <set>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <glm/vec3.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::light::internal {

/**
 * The images the scene's lights read, each read once for each path and converted into ACEScg.
 *
 * An image is read from an EXR file, in the colour space its chromaticities name. Chromaticities
 * of any other space are read as linear Rec. 709, and reported. A file that cannot be read is
 * reported, once for each path, and gives no image.
 **/
class Textures final {
 public:
    /** The configuration outlives the textures. The logger may be null. **/
    Textures(const colour::Config & config, boost::shared_ptr<log::Logger> logger);

    /** The place among the images of the image a path names, or kNoPrimitive when unread. **/
    std::uint32_t load(const std::string & path);

    /** Whether the file an image was read from says that it is a cube map. **/
    bool cube(std::uint32_t image) const;

    /**
     * Gives an image the two ways a dome light's texel is drawn, as kernel::light::Image says. An
     * image whose texels give a way nothing to draw by is drawn in proportion to the solid angle
     * each covers, uniformly over the sphere.
     **/
    void drawable(std::uint32_t image);

    /** The integral over the sphere of a dome's image's luminance, by each texel's solid angle. **/
    float integral(std::uint32_t image) const;

    /** The average of an image's texels' luminance. **/
    float average(std::uint32_t image) const;

    /** Reports a light that cannot be lit, once for each message. **/
    void report(const std::string & message);

    const std::vector<kernel::light::Image> & images() const;
    const std::vector<glm::vec3> & texels() const;
    const std::vector<float> & sums() const;

 private:
    /** The solid angle each texel of a row of an image covers, the rows from the top. **/
    static std::vector<float> texelAngles(const kernel::light::Image & image);

    /** Appends the sums of a way of drawing an image's texels by their weights, row by row. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the rows' first sum, then the columns'
    void appendWay(const kernel::light::Image & image, const std::vector<float> & weights, std::uint32_t & rows, std::uint32_t & columns);

    const colour::Config & config_;
    boost::shared_ptr<log::Logger> logger_;
    std::map<std::string, std::uint32_t> byPath_;
    std::set<std::string> reported_;
    std::vector<kernel::light::Image> images_;
    std::vector<bool> cubes_;
    std::vector<glm::vec3> texels_;
    std::vector<float> sums_;
};

};  // namespace v3d::moya::light::internal
