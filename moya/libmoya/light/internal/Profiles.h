/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/kernel/light/Profile.h>

#include <cstdint>
#include <map>
#include <string>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::light::internal {

/**
 * The IES profiles the scene's lights read, each file read once. A file that cannot be read is
 * reported, once for each path, and gives no profile. Every profile loaded is held until the
 * profiles are destroyed.
 *
 * Without normalize, a profile multiplies a light's emission by its candela. With it, the candela
 * are divided by their mean over the sphere, so the profile shapes the light and leaves its power
 * as it was. Directions the profile's vertical angles do not reach count as dark in that mean.
 **/
class Profiles final {
 public:
    /** The logger may be null. **/
    explicit Profiles(boost::shared_ptr<log::Logger> logger);

    /**
     * The place among the profiles of the profile a file gives, with an angle scale and
     * normalised or not, or kNoPrimitive when the file cannot be read.
     **/
    std::uint32_t load(const std::string & path, float angleScale, bool normalize);

    /**
     * The mean over the sphere of what a profile multiplies a light's emission by, which the
     * light's power is weighed by.
     **/
    float mean(std::uint32_t profile) const;

    /**
     * The profiles some places among those loaded name, as kernel code reads them, in their order,
     * with the photometry of the files they read, each once, in the order of its first use.
     **/
    struct Packed {
        std::vector<kernel::light::Profile> profiles;
        std::vector<float> photometry;
    };

    /** The profiles some places among those loaded name. **/
    Packed pack(const std::vector<std::uint32_t> & uses) const;

 private:
    /** The mean over the sphere of a profile, summed over bands of equal latitude. **/
    float meanOf(const kernel::light::Profile & profile) const;

    boost::shared_ptr<log::Logger> logger_;
    std::map<std::string, kernel::light::Profile> byPath_;
    std::vector<kernel::light::Profile> profiles_;
    std::vector<float> photometry_;

    /** How many values of the photometry each file read holds, by the first of them. **/
    std::map<std::uint32_t, std::uint32_t> blocks_;
};

};  // namespace v3d::moya::light::internal
