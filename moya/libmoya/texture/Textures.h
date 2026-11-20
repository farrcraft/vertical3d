/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <moya/libmoya/device/Buffer.h>
#include <moya/libmoya/device/cpu/Textures.h>
#include <moya/libmoya/kernel/texture/Image.h>
#include <moya/libmoya/texture/Cache.h>
#include <moya/libmoya/texture/Options.h>
#include <moya/libmoya/texture/Request.h>

#include <cstdint>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include <boost/shared_ptr.hpp>

namespace v3d::log {
class Logger;
};  // namespace v3d::log

namespace v3d::moya::colour {
class Config;
};  // namespace v3d::moya::colour

namespace v3d::moya::device {
class Device;
};  // namespace v3d::moya::device

namespace v3d::moya::texture {

/**
 * The textures of every image a scene's programs read, found, made and opened in a cache.
 *
 * - **A file is found** as found() says, along the options' search path.
 * - **A UDIM image** holds `<UDIM>` in its path. Each tile from 1001 to 1100 that is found is read,
 *   ten to a row of v, and a tile not found reads as nothing. A UDIM image of no tile is logged.
 * - **A tiled EXR file of every level rounded down is read as it is**, when it is read as data or
 *   its chromaticities are ACEScg's. Any other image is made into one by texture::make(), in the
 *   options' directory, named by a hash of its bytes, its colour space and whether it is data. A
 *   texture made before from the same image is read again.
 * - **An image that is not found, or cannot be made or opened, is logged**, and reads as nothing,
 *   so its node gives its default.
 *
 * Images of one file, colour space and kind share their texture.
 **/
class Textures final {
 public:
    /**
     * @param images every image the programs read, by the number its node names it with
     **/
    Textures(device::Device & device, const std::vector<Request> & images, const Options & options, const colour::Config & config,
        const boost::shared_ptr<log::Logger> & logger);
    Textures(const Textures &) = delete;
    Textures & operator=(const Textures &) = delete;

    /** The CPU's texture access, reading through the cache. Valid while this object lives. **/
    device::cpu::Textures view() const;

    /** Logs each file a texel could not be read from while rendering. **/
    void report() const;

    const Cache & cache() const {
        return *cache_;
    }

 private:
    /** The texture of one file, found, made and opened, or kNoTexture. **/
    std::uint32_t textureOf(const std::string & file, const Request & request);

    /** The path of the texture an image is made into, made when it is not there. **/
    std::string madeFrom(const std::string & path, const Request & request);

    Options options_;
    const colour::Config & config_;
    boost::shared_ptr<log::Logger> logger_;
    boost::shared_ptr<Cache> cache_;

    /** Each texture opened, by its file and how it is read. **/
    std::map<std::pair<std::string, std::string>, std::uint32_t> opened_;

    std::vector<kernel::texture::Image> images_;
    std::vector<std::uint32_t> tiles_;
    boost::shared_ptr<device::Buffer> imageBuffer_;
    boost::shared_ptr<device::Buffer> tileBuffer_;
    boost::shared_ptr<device::Buffer> textureBuffer_;
};

};  // namespace v3d::moya::texture
