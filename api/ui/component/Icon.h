/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../Component.h"

#include "../../render/realtime/Handle.h"

#include <boost/shared_ptr.hpp>

namespace v3d::ui::component {

/**
 * An Icon Component.
 *
 * An icon names the image it draws and holds the texture whatever uploaded that image
 * put there, which is unset until something has. Its size is the component's own: a
 * handle is a slot id rather than a picture, so there is nothing here to measure.
 */
class Icon : public Component {
 public:
    /**
     * @param source the name of the image, for whatever resolves sources to textures
     */
    explicit Icon(const std::string& source);
    ~Icon();

    /**
     * @return the name of the image the icon draws
     */
    std::string_view source() const;

    /**
     * @return the texture the icon draws with, unset until something has uploaded source()
     */
    v3d::render::realtime::TextureHandle texture() const noexcept;
    /**
     * @param tex a handle from the renderer that uploaded source()
     */
    void texture(const v3d::render::realtime::TextureHandle& tex) noexcept;

 private:
    std::string source_;
    v3d::render::realtime::TextureHandle texture_;
};


};  // namespace v3d::ui::component
