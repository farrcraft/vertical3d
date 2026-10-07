/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/log/Logger.h>
#include <api/render/realtime/vulkan/device/Instance.h>
#include <api/render/realtime/vulkan/device/Surface.h>

#include <SDL3/SDL.h>
#include <SDL3/SDL_vulkan.h>

#include <string>

#include <boost/shared_ptr.hpp>

namespace v3d::render::realtime {
/**
 * The window everything is drawn into, and the Vulkan instance and surface that present to it.
 *
 * Every window is a Vulkan window: an app presents through a swapchain whether it draws in two
 * dimensions or three. A 2D app differs from a 3D one in what its passes request - an
 * orthographic camera and no depth - not in the window.
 **/
class Window final {
 public:
    /**
     * @param logger
     **/
    explicit Window(const boost::shared_ptr<v3d::log::Logger>& logger) noexcept;

    /**
     * Create the window, the vulkan instance, and the surface that presents to it.
     *
     * @throw std::runtime_error if the vulkan loader or SDL's extension list is missing
     * @return whether the window itself could be created
     **/
    bool create(int width, int height);

    /**
     * @return whether create() has succeeded and destroy() has not been called since
     **/
    bool created() const noexcept;

    /**
     **/
    void destroy();

    /**
     **/
    SDL_Window* sdl() noexcept;

    /**
     * @return the vulkan instance the window was created against
     **/
    boost::shared_ptr<vulkan::device::Instance> instance() const;

    /**
     * @return the vulkan surface the window presents to
     **/
    boost::shared_ptr<vulkan::device::Surface> surface() const;

    /**
     * Record a size the window has already been given.
     *
     * The loop calls this when SDL reports a resize that has already happened. It updates
     * what width() and height() report and does not change the window. To ask for a new
     * size, call request().
     **/
    void resize(int width, int height) noexcept;

    /**
     * Ask the window to become a size, for example to restore a remembered one.
     *
     * The recorded size is not written here. SDL responds with a resize event and the loop
     * calls resize() with it, so a size the window manager refused or adjusted is never
     * reported as the window's size.
     **/
    void request(int width, int height);

    /**
     **/
    int width() const noexcept;

    /**
     **/
    int height() const noexcept;

    /**
     * Set the window caption.
     * @param cap the new window caption
     **/
    void caption(const std::string_view& cap);

    /**
     * Whether the window has keyboard focus.
     *
     * An app that steers with the pointer checks this, so that a move made over whatever
     * the player alt tabbed to does not turn the view.
     *
     * @return false when the window is not the one being typed into, and while there is
     *         no window at all
     **/
    bool focused() const;

    /**
     * Start or stop the platform composing text. SDL_EVENT_TEXT_INPUT arrives only while
     * it is on.
     *
     * SDL3 sends no text events until text input is started, so a ui text box would
     * otherwise receive the keys and never the characters. It is off when the window is
     * created, and is turned on only while something that takes typing is focused. Starting
     * it raises an on screen keyboard where the platform has one, and stopping it lowers it.
     *
     * ui::shell::Keyboard tracks the focus and calls this, so an app that routes its
     * keyboard through it does not call this itself.
     *
     * @param on whether to compose
     * @return whether the platform accepted the change; false for a window not yet created
     **/
    bool textInput(bool on);

    /**
     * @return whether the platform is composing text
     **/
    bool textInput() const;

    /**
     * Put the mouse in relative mode, or take it out.
     *
     * In relative mode the cursor is hidden and held in the window, and a motion event's
     * motion() is how far the mouse moved however near an edge it is, which mouselook
     * reads. The platform releases the mouse while the window is not focused and captures
     * it again on focus. An app therefore turns the mode off only for its own reasons, such
     * as a menu that needs a pointer.
     *
     * @param on whether to enter relative mode
     * @return whether the platform accepted the change; false for a window not yet created
     **/
    bool relativeMouse(bool on);

    /**
     * @return whether the mouse is in relative mode for this window
     **/
    bool relativeMouse() const;

    /**
     * Toggle mouse cursor visibility
     * @param state whether to enable or disable
     **/
    static void cursor(bool state);

    /**
     * Move the mouse cursor to a new position in the window
     **/
    void warpCursor(int x, int y);

 private:
    SDL_Window* window_;
    boost::shared_ptr<vulkan::device::Instance> instance_;
    boost::shared_ptr<vulkan::device::Surface> surface_;
    std::string caption_;
    int width_;
    int height_;
    bool vulkanLoaded_;
    bool created_;
    boost::shared_ptr<v3d::log::Logger> logger_;
};

};  // namespace v3d::render::realtime
