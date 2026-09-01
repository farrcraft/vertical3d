/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <functional>
#include <string>

#include "Container.h"
#include "Engine.h"
#include "component/menu/Menu.h"

#include "../render/realtime/Canvas.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui {

    /**
     * Draws the ui onto a canvas of quads.
     *
     * A panel, a highlight and a line of text are all the batched quad of ADR-0005, so the
     * whole ui is added to whatever canvas the app is already filling and costs the frame no
     * pass and no draw of its own.
     *
     * Text is the caller's to lay out. This library knows where a label goes and how wide the
     * thing around it has to be; turning a string into glyph quads belongs to v3d::font and to
     * whatever atlas the app loaded, so those two are handed in as callbacks.
     *
     * Only menus are drawn. The other components in this library have no loader, so nothing
     * can construct one to be drawn - see docs/LuxaAudit.md.
     **/
    class ComponentRenderer {
     public:
        /**
         * How wide a string will be when it is drawn, in pixels.
         **/
        typedef std::function<float(const std::string&)> Measure;

        /**
         * Draw a string with its pen on the baseline at the given position.
         **/
        typedef std::function<void(const std::string&, const glm::vec2&, const glm::vec4&)> Write;

        /**
         * What the ui cannot work out from the components alone.
         *
         * These belong to the theme, which cannot supply them: v3d::ui::style::Theme loads,
         * but its properties are write-only - prop::Color has no accessor and nothing parses
         * one out of a config. Defaults live here until the style properties can be read.
         **/
        struct Style final {
            Style() noexcept;

            float lineHeight;      /**< the baseline to baseline distance of one menu item **/
            float padding;         /**< the gap between the text and the panel around it **/
            glm::vec4 panel;       /**< the background the menu is drawn on **/
            glm::vec4 border;      /**< the panel's outline **/
            glm::vec4 text;        /**< an ordinary item's label **/
            glm::vec4 activeText;  /**< the label of the item navigation is on **/
            glm::vec4 highlight;   /**< what is drawn behind that item **/
        };

        /**
         * @param measure how wide a string is when the app draws it
         * @param write how the app draws a string
         **/
        ComponentRenderer(const Measure& measure, const Write& write);

        /**
         * @return the colours and metrics the ui is drawn with, to be changed in place
         **/
        Style& style() noexcept;

        /**
         * Draw every visible container of a ui engine.
         **/
        void draw(v3d::render::realtime::Canvas* canvas, const Engine& ui) const;

        /**
         * Draw every visible component of one container.
         **/
        void draw(v3d::render::realtime::Canvas* canvas, const Container& container) const;

        /**
         * Draw the active level of a menu, centred on the canvas.
         *
         * It is the active level rather than the menu itself that is drawn: descending into
         * a submenu replaces what is on screen, and the parent stays where it was.
         **/
        void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Menu>& menu) const;

     private:
        Measure measure_;
        Write write_;
        Style style_;
    };

};  // namespace v3d::ui
