/**
 * Vertical3D
 * Copyright (c) 2023 Joshua Farr (josh@farrcraft.com)
 **/

#include "Overlay.h"

#include <GL/glew.h>

namespace v3d::render::realtime::operation {

    /**
     **/
    Overlay::Overlay(boost::shared_ptr<v3d::ui::Overlay> overlay) :
        overlay_(overlay) {

    }

    /**
     **/
    bool Overlay::run(boost::shared_ptr<Context> context) {
        if (overlay_->mode() == v3d::ui::Overlay::MODE_COLOR) {
            glm::vec3 overlay_color = overlay_->color();
            glClearColor(overlay_color[0], overlay_color[1], overlay_color[2], 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        }

        return true;
    }

};  // namespace v3d::render::realtime::operation
