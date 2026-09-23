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
 * An Icon Component.
 *
 * An icon names the image it draws and holds what that name resolved to, which is nothing
 * until something has resolved it. Its size is the component's own: a handle is a slot id
 * rather than a picture, so there is nothing here to measure.
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
     * Name a different image, which is how an icon changes what it shows.
     *
     * The image the old name resolved to is dropped rather than kept, so the icon draws
     * nothing until the new name is resolved rather than the old picture under the new name.
     * ui::Engine::resolveImages() is what resolves it, and resolves it again after that.
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
