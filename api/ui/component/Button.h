/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>
#include <api/render/realtime/Handle.h>
#include <api/ui/Component.h>

#include <string>

namespace v3d::ui::component {
/**
 * A vGUI Button
 *
 * A toggle button does not own the state it shows, for the reason a check menu item does
 * not: pressing one sends its command and marks nothing, and whatever answers the command
 * sets checked(). See ADR-0019.
 *
 * A button that names an icon is drawn as that image instead of as its label, and holds
 * the texture whatever uploaded the image put there. It keeps its label either way, which
 * is what a strip measures before anything has been resolved. See ADR-0020.
 *
 * A button that cannot be used is not a state here. That is Component::enabled(), which
 * lasts, where a state lasts as long as the cursor is where it is. See ADR-0059.
 */
class Button : public Component {
 public:
    Button();

    /**
        * button state enumeration
        */
    typedef enum {
        STATE_NORMAL,  /**< Normal Button State **/
        STATE_HOVER,   /**< Mouse is hovering over button **/
        STATE_PRESS    /**< Button is being clicked **/
    } ButtonState;

    /**
      * Set the button's label
      * @param str the new label text
      */
    void label(const std::string& str);
    /**
     * Get the button's label
     * @return the current label text
     */
    std::string_view label() const;
    /**
     * Get the current button state
     * @return current state
     */
    ButtonState state() const;
    /**
     * Set the current button state
     * @param s the new state
     */
    void state(ButtonState s);

    /**
     * Set the event pressing the button sends.
     * @param destination the event destination
     **/
    void event(const v3d::event::Event& destination);
    /**
     * Get the event bound to the button
     * @return the event
     **/
    v3d::event::Event event() const;

    /**
     * Set the image the button draws instead of its label.
     * @param source the name of the image, for whatever resolves sources to textures
     **/
    void icon(const std::string& source);
    /**
     * @return the name of the image the button draws, empty when it is a labelled one
     **/
    std::string_view icon() const;

    /**
     * @return the texture the icon draws with, unset until something has uploaded icon()
     **/
    v3d::render::realtime::TextureHandle texture() const noexcept;
    /**
     * @param tex a handle from the renderer that uploaded icon()
     **/
    void texture(const v3d::render::realtime::TextureHandle& tex) noexcept;

    /**
     * Set whether the button shows a mark when it is checked.
     * @param on whether it is a toggle rather than a plain button
     **/
    void toggle(bool on);
    /**
     * @return whether the button is a toggle
     **/
    bool toggle() const;

    /**
      * Set whether a toggle button draws its mark.
      * @param on whether the button is checked
      */
    void checked(bool on);
    /**
      * Get whether the button is checked. A button that is not a toggle is never checked.
      * @return whether the button draws its mark
      */
    bool checked() const;

 private:
    std::string label_;
    std::string icon_;
    v3d::render::realtime::TextureHandle texture_;
    ButtonState state_;
    v3d::event::Event event_;
    bool toggle_;
    bool checked_;
};

};  // namespace v3d::ui::component
