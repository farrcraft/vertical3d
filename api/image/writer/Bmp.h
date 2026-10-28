/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/image/Writer.h>

#include <optional>
#include <string>

namespace v3d::image::writer {

class Bmp final : public v3d::image::Writer {
 public:
    Bmp(const boost::shared_ptr<v3d::log::Logger> & logger);
    ~Bmp() = default;

    std::optional<std::string> encode(const boost::shared_ptr<Image> & img) override;
};
};  // namespace v3d::image::writer
