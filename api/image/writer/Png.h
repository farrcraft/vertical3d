/*
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 */

#pragma once

#include <api/image/Writer.h>

#include <optional>
#include <string>

/**
 * A writer for each image format, each encoding an image.
 **/
namespace v3d::image::writer {
/**
 * Encodes an image as a png file, through libpng.
 **/
class Png : public v3d::image::Writer {
 public:
    /**
     **/
    explicit Png(const boost::shared_ptr<v3d::log::Logger> & logger);
    ~Png() = default;

    std::optional<std::string> encode(const boost::shared_ptr<Image> & img) override;
};

};  // namespace v3d::image::writer
