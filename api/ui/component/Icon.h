/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include "../Component.h"

#include "../../gl/GLTexture.h"

#include <boost/shared_ptr.hpp>

namespace v3d::ui::component {

    /**
     * An Icon Component.
     * Icons consist of a single texture image. Their size comes from the texture's
     * dimensions and not the size set in the component.
     */
    class Icon : public Component {
     public:
        explicit Icon(boost::shared_ptr<v3d::gl::GLTexture> texture);
        ~Icon();

     private:
        boost::shared_ptr<v3d::gl::GLTexture> texture_;
    };


};  // namespace v3d::ui::component
