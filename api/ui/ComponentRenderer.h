/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <functional>
#include <string>

#include "Container.h"
#include "Engine.h"
#include "component/Toolbar.h"
#include "component/menu/Menu.h"
#include "component/menu/MenuBar.h"

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
     * Drawing is also what lays the ui out: every component is left holding the bounds it
     * was drawn in, which is what the cursor is tested against, per ADR-0019.
     *
     * Only menus, menu bars and toolbars are drawn. The other components in this library
     * have no loader, so nothing can construct one to be drawn - see docs/LuxaAudit.md.
     *
     * The strips stack in the order a container lists them. A menu bar takes the top of the
     * canvas, a top toolbar takes a band under whatever is already there, and a left toolbar
     * runs down the side of what is left. insets() is that arithmetic on its own, for an app
     * that has to know what area it is left with before anything has been drawn.
     *
     * A menu bar is nonetheless drawn last, because an open menu drops a panel over the
     * strips below it.
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
            float barHeight;       /**< how tall the strip of a menu bar or a toolbar is **/
            float panelPadding;    /**< the gap above and below the items of a dropped panel **/
            glm::vec4 panel;       /**< the background the menu is drawn on **/
            glm::vec4 border;      /**< the panel's outline **/
            glm::vec4 text;        /**< an ordinary item's label **/
            glm::vec4 activeText;  /**< the label of the item navigation is on **/
            glm::vec4 highlight;   /**< what is drawn behind that item **/
            glm::vec4 hover;       /**< what is drawn behind a toolbar button the cursor is on **/
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

        /**
         * Draw a menu bar - the strip across the top, and the open menu with any flyout the
         * cursor has descended into drawn under it. It takes the top of the canvas, which is
         * where its panels are dropped from.
         **/
        void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::MenuBar>& bar) const;

        /**
         * Draw a toolbar - a row of buttons across the canvas, or a column down its side.
         *
         * @param corner the strip's top left, which is where the edge it runs along has
         *      room for it once the strips before it have taken theirs
         **/
        void draw(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Toolbar>& bar,
            const glm::vec2& corner) const;

        /**
         * How much of the canvas the strips of one container cover - the band along the top
         * and the one down the left side, in pixels. What is left is what the app has to
         * draw in.
         *
         * @return the left inset in x and the top inset in y
         **/
        glm::vec2 insets(const Container& container) const;

        /**
         * The insets of every visible container of a ui engine.
         **/
        glm::vec2 insets(const Engine& ui) const;

     private:
        /**
         * Draw one dropped panel of a menu bar, leaving every item holding its own row.
         *
         * @param origin where the panel's top left corner would go, before it is moved to
         *      keep it on the canvas
         **/
        void panel(v3d::render::realtime::Canvas* canvas, const boost::shared_ptr<component::Menu>& menu,
            const glm::vec2& origin) const;

        /**
         * How wide a toolbar's widest label is, which is what sizes a column and what a row
         * has no use for.
         **/
        float widest(const component::Toolbar& bar) const;

        Measure measure_;
        Write write_;
        Style style_;
    };

};  // namespace v3d::ui
