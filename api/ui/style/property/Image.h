/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../Property.h"

#include "../../../render/realtime/Handle.h"

#include <boost/shared_ptr.hpp>

namespace v3d::ui::style::prop {

/**
 * A style property that defines an image.
 */
class Image : public Property {
 public:
    Image(const std::string& name, const std::string& src);
    ~Image();

    /**
     * Get the texture associated with the image property
     * @return the handle, which is unset until something has uploaded the source
     */
    v3d::render::realtime::TextureHandle texture() const noexcept;
    /**
     * Get the name of the image source
     * @return the image source name
     */
    std::string_view source() const;
    /**
     * Set the texture the image property draws with
     * @param tex a handle from the quad renderer that uploaded source()
     */
    void texture(const v3d::render::realtime::TextureHandle& tex) noexcept;

 private:
    std::string source_;
    v3d::render::realtime::TextureHandle texture_;
};

};  // end namespace v3d::ui::style::prop
