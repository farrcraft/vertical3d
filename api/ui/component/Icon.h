/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Component.h>
#include <api/ui/Image.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::ui::component {

/**
 * A component that draws an image.
 *
 * An icon names the image it draws and holds what that name resolved to, which is unset
 * until something has resolved it. Its size comes from its layout, and an Auto size is a
 * square the height of a strip. A texture handle is a slot id, not a picture, so the icon
 * has no image size to measure.
 */
class Icon : public Component {
 public:
    /**
     * @param source the name of the image, for whatever resolves sources to images
     */
    explicit Icon(const std::string& source);
    ~Icon();

    /**
     * @return the name of the image the icon draws
     */
    std::string_view source() const;
    /**
     * Name a different image for the icon to show.
     *
     * The image the old name resolved to is dropped, so the icon draws nothing until the new
     * name is resolved. ui::Interface::resolveComponentImages() resolves this icon alone, and
     * every later resolveImages() resolves it from the new name.
     *
     * @param name the name of the image
     */
    void source(const std::string& name);

    /**
     * @return what source() resolved to, unset until something has resolved it
     */
    const v3d::ui::Image& image() const noexcept;
    /**
     * @param resolved what source() resolved to
     */
    void image(const v3d::ui::Image& resolved) noexcept;

 private:
    std::string source_;
    v3d::ui::Image image_;
};


};  // namespace v3d::ui::component
