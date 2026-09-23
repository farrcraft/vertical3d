/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Image.h>
#include <api/ui/style/Property.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::ui::style::property {

/**
 * A style property that names an image, and holds what the name resolved to.
 */
class Image : public Property {
 public:
    Image(const std::string& name, const std::string& src);
    ~Image();

    /**
     * Get what the source resolved to
     * @return the image, which is unset until something has resolved the source
     */
    const v3d::ui::Image& image() const noexcept;
    /**
     * Get the name of the image source
     * @return the image source name
     */
    std::string_view source() const;
    /**
     * Set what the image property draws
     * @param resolved what source() resolved to
     */
    void image(const v3d::ui::Image& resolved) noexcept;

 private:
    std::string source_;
    v3d::ui::Image image_;
};

};  // end namespace v3d::ui::style::property
