/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../../api/gl/GLTexture.h"

#include <boost/shared_ptr.hpp>
#include <glm/glm.hpp>

#include "../../api/ui/Overlay.h"
#include "../../api/ui/style/Theme.h"

namespace Luxa {

    /**
     * A class for handling all of the actual OpenGL vGUI rendering.
     */
    class ComponentRenderer {
     public:
            explicit ComponentRenderer(const boost::shared_ptr<v3d::log::Logger>& logger);
            explicit ComponentRenderer();
            ~ComponentRenderer();

            void overlay(const v3d::ui::Overlay & o);

            /**
             * Draw the ui
             */
            void draw();
            /**
             * Resize the rendering area
             * @param width the new width
             * @param height the new height
             */
            void resize(int width, int height);
            /**
             * Draw a texture mapped quad
             * @param texture the texture to map on the quad
             * @param position_x the x coordinate of the upper left corner of the quad on the screen
             * @param position_y the y coordinate of the upper left corner of the quad on the screen
             * @param width the width of the quad in pixels
             * @param height the height of the quad in pixels
             */
            bool drawTexturedQuad(boost::shared_ptr<v3d::gl::GLTexture> texture, float position_x, float position_y, float width, float height);
            /**
             * Draw a texture
             * @param texture the texture to draw
             * @param position the position to draw the texture at
             */
            bool drawTexture(boost::shared_ptr<v3d::gl::GLTexture> texture, glm::vec2 position);

            /**
             * Set the rendering color
             * @param c the color to use for rendering
             */
            void color(const glm::vec3 & c);

            /**
             * Translate to the requested position relative to the current position
             * @param pos the new relative position to move to
             */
            void position(const glm::vec2 & pos);
            /**
             * Push the rendering matrix state
             */
            void push();
            /**
             * Pop the rendering matrix state
             */
            void pop();
            /**
             * Clear the rendering matrix state
             */
            void clear();
            /**
             * Get the width of the managed component area.
             * This is usually the same as the window canvas width
             * @return the current width
             */
            unsigned int width() const;
            /**
             * Get the height of the managed component area.
             * This is usually the same as the window canvas height
             * @return the current height
             */
            unsigned int height() const;

            void prepare();
            void post();

     private:
            int width_;
            int height_;
            v3d::ui::Overlay overlay_;
    };

};  // end namespace Luxa
