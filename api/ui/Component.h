/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include <glm/glm.hpp>

#include "style/Theme.h"

#include "../../api/type/Bound2D.h"

namespace v3d::ui {

    /**
     * A vGUI Component
     * All UI components are all derived from this class.
     */
    class Component {
     public:
        static unsigned int lastID;

        Component();
        virtual ~Component();

        /**
         * Get the id of the component
         * @return the id
         */
        unsigned int id() const;

        /**
         * Set the position of the component.
         * @param pos the new position
         */
        void position(const glm::vec2& pos);
        /**
         * Set the size of the component.
         * @param s the new size
         */
        void size(const glm::vec2& s);
        /**
         * Get the current position of the component.
         * @return the current position
         */
        glm::vec2 position() const;
        /**
         * Get the current size of the component.
         * @return the current size
         */
        glm::vec2 size() const;
        /**
         * Get the component's bounding volume
         * @return the component's bounding box
         */
        v3d::type::Bound2D bound() const;
        /**
         * Get the component's z index depth value
         * @return the component's zindex
         */
        unsigned int depth() const;
        /**
         * Set the style name of the component
         * @param str the style name
         */
        void style(const std::string& str);
        /**
         * Get the style name of the component
         * @return the style name
         */
        std::string style() const;
        /**
         * Get whether the component is visible or not
         * @return true if the component is visible
         */
        bool visible() const;
        /**
         * Set the visibility state of the component
         * @param vis the new visibility setting
         */
        void visible(bool vis);
        /**
         * Get the component name
         * @return the component name
         */
        std::string name() const;
        /**
         * Set the component name
         * @param str the new component name
         */
        void name(const std::string& str);

     private:
        glm::vec2 position_;
        glm::vec2 size_;
        unsigned int zIndex_;
        unsigned int id_;
        std::string style_;
        std::string name_;
        bool visible_;
    };

};  // end namespace v3d::ui
