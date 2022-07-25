/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include <glm/glm.hpp>

#include "style/Theme.h"

#include "../../api/type/Bound2D.h"
#include "../../api/command/CommandInfo.h"
#include "../../api/event/EventListener.h"

/**
 * Luxa library namespace.
 */
namespace Luxa {

    class ComponentRenderer;
    class ComponentManager;

    /**
     * A vGUI Component
     * All UI components are all derived from this class.
     */
    class Component : public v3d::event::EventListener {
     public:
            static unsigned int lastID;

            explicit Component(ComponentManager * cm);
            virtual ~Component();

            /**
             * Draw the component.
             * @param renderer the renderer to use for drawing
             * @param theme the active theme to use for drawing
             */
            virtual void draw(ComponentRenderer * renderer, const boost::shared_ptr<Theme> & theme) const;

            /**
             * Get the id of the component
             * @return the id
             */
            unsigned int id() const;

            /**
             * Set the position of the component.
             * @param pos the new position
             */
            void position(const glm::vec2 & pos);
            /**
             * Set the size of the component.
             * @param s the new size
             */
            void size(const glm::vec2 & s);
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
            void style(const std::string & str);
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
            void name(const std::string & str);
            /**
             * Command handler for UI events. It is also derived in some derived Components (e.g. Menu).
             * @param cmd the CommandInfo object describing the command being executed
             * @param param the parameter passed to the executed command
             * @return whether the command successfully executed
             */
            virtual bool exec(const v3d::command::CommandInfo & cmd, const std::string & param);
            /**
             * Event notification method.
             * Override from v3D::EventListener providing a way for derived components to react to events.
             * @param e the event that has occurred.
             */
            virtual void notify(const v3d::event::EventInfo & e);

     protected:
            ComponentManager * manager_;

     private:
            glm::vec2 position_;
            glm::vec2 size_;
            unsigned int zIndex_;
            unsigned int id_;
            std::string style_;
            std::string name_;
            bool visible_;
    };

};  // end namespace Luxa
