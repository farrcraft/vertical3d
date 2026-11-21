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
 * Every image read is held until the textures are destroyed, so a light made again reads its
 * path's image as it was read.
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

    /** The integral over the sphere of a dome's image's luminance, by each texel's solid angle. **/
    float integral(std::uint32_t image) const;

    /** The average of an image's texels' luminance. **/
    float average(std::uint32_t image) const;

    /** Reports a light that cannot be lit, once for each message. **/
    void report(const std::string & message);

    /** An image a light reads, and whether a dome light draws its texels. **/
    struct Use {
        std::uint32_t image { 0 };
        bool drawn { false };
    };

    /**
     * The images some uses name, as kernel code reads them, each once, in the order of its first
     * use, with its texels. places holds the place among them of each use's image. Each use that
     * is drawn gives its image the two ways a dome light's texel is drawn, as
     * kernel::light::Image says, appended to the sums. An image whose texels give a way nothing to
     * draw by is drawn in proportion to the solid angle each covers, uniformly over the sphere.
     **/
    struct Packed {
        std::vector<kernel::light::Image> images;
        std::vector<glm::vec3> texels;
        std::vector<float> sums;
        std::vector<std::uint32_t> places;
    };

    /** The images some uses name, from those read so far. **/
    Packed pack(const std::vector<Use> & uses);

 private:
    /** The sums of an image's two ways of being drawn, each its rows' and then its columns'. **/
    struct Ways {
        std::vector<float> rows;
        std::vector<float> columns;
        std::vector<float> compensatedRows;
        std::vector<float> compensatedColumns;
    };

    /** The solid angle each texel of a row of an image covers, the rows from the top. **/
    static std::vector<float> texelAngles(const kernel::light::Image & image);

    /** The sums of a way of drawing an image's texels by their weights, row by row. **/
    // NOLINTNEXTLINE(bugprone-easily-swappable-parameters) - the rows' sums, then the columns'
    static void wayOf(const kernel::light::Image & image, const std::vector<float> & weights, std::vector<float> & rows, std::vector<float> & columns);

    /** The two ways an image is drawn, found once for each image. **/
    const Ways & waysOf(std::uint32_t image);

    const colour::Config & config_;
    boost::shared_ptr<log::Logger> logger_;
    std::map<std::string, std::uint32_t> byPath_;
    std::set<std::string> reported_;
    std::vector<kernel::light::Image> images_;
    std::vector<bool> cubes_;
    std::vector<glm::vec3> texels_;
    std::map<std::uint32_t, Ways> ways_;
};

};  // namespace v3d::moya::light::internal
