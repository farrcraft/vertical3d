/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
**/

#include "ViewPort.h"
#include "../v3dlibs/core/Scene.h"

#include <boost/make_shared.hpp>

namespace v3d {

    ViewPort::ViewPort(const boost::shared_ptr<Hookah::Window>& window, const boost::shared_ptr<v3d::core::Scene>& scene) :
        window_(window),
        scene_(scene) {
        camera_ = boost::make_shared<v3d::type::Camera>();
    }

    ViewPort::~ViewPort() {
    }

    boost::shared_ptr<v3d::type::Camera> ViewPort::camera() {
        return camera_;
    }

    void ViewPort::resize(int width, int height) {
        rc_.resize(width, height);
    }

    void ViewPort::invalidate() {
        window_->invalidate();
    }

    void ViewPort::draw(Hookah::Window* window) {
        rc_.prepare(camera_);
        /*
            if (showFlags_ & SHOW_GRID)
            {
                glDisable(GL_LIGHTING);
                glPushMatrix();
                _grid.draw(_camera);
                glPopMatrix();
                glEnable(GL_LIGHTING);
            }
        */
        rc_.render(scene_);
    }
};  // namespace v3d
