/**
/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <glm/glm.hpp>

namespace v3d::ui {

    /**
     * The Overlay is used to specify the background attributes of the UI.
     * Since the UI can be drawn on top of other content we don't necessarily want to do the
     * normal clearing of the background color when drawing. The overlay is used to tell the
     * renderer how to handle the background. It can be transparent (do nothing), or a normal
     * background color.
     */
    class Overlay {
     public:
        /**
            * Overlay Mode
            */
        enum Mode {
            MODE_TRANSPARENT,
            MODE_COLOR
        };

        /**
            * Default Constructor
            */
        Overlay();
        /**
            * Construct an overlay with the given mode
            * @param m the mode to use
            */
        explicit Overlay(Mode m);
        /**
            * Construct a color overlay with the given color
            * @param c the color of the overlay
            */
        explicit Overlay(const glm::vec3& c);
        /**
            * Set the overlay's color
            * @param c the new color
            */
        void color(const glm::vec3& c);
        /**
            * Get the overlay's color
            * @return the current overlay color
            */
        glm::vec3 color() const;
        /**
            * Get the overlay mode
            * @return the current overlay mode
            */
        Mode mode() const;

     private:
        Mode mode_;
        glm::vec3 color_;
    };

};  // end namespace v3d::ui
