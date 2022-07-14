/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "HWRenderContext.h"
#include "../v3dlibs/hookah/Window.h"
#include "../v3dlibs/core/Scene.h"

namespace v3d {

    /**
     * A ViewPort renders a Scene through a Camera using a RenderContext. It is attached 
     * to a Window instance. It also handles rendering of ManipulatorDecoration's and 
     * ConstructionTool's along with any other ViewPortDecorator's. Hit-testing is 
     * supported by the RenderContext given a Camera and set of (x, y) hit coordinates to use.
     */
    class ViewPort {
     public:
            /**
             * Constructor.
             * @param window the window object
             * @param scene the scene to view.
             */
            ViewPort(const boost::shared_ptr<Hookah::Window> & window, const boost::shared_ptr<v3d::core::Scene> & scene);
            ~ViewPort();

            typedef enum VisibleFilter {
                SHOW_GRID = 1,
                SHOW_CAMERA,
                SHOW_HANDLE,
                SHOW_LIGHT,
                SHOW_MESH
            } VisibleFilter;

            /**
             * Draw the viewport. This is a callback method.
             * @param window a pointer to the parent window
             */
            void draw(Hookah::Window * window);
            /**
             * Resize the viewport.
             * @param width the new viewport width
             * @param height the new viewport height
             */
            void resize(int width, int height);

            boost::shared_ptr<v3d::type::Camera> camera();

            /**
             * Invalidate the viewport contents.
             */
            void invalidate();

     private:
            boost::shared_ptr<v3d::core::Scene> scene_;
            boost::shared_ptr<v3d::type::Camera> camera_;
            HWRenderContext rc_;

            unsigned int showFlags_;
            glm::vec3 backgroundColor_;
            std::string name_;
            boost::shared_ptr<Hookah::Window> window_;
    };

};  // namespace v3d

