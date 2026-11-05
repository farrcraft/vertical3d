/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/engine/Engine.h>
#include <api/event/kind/DisplayScale.h>
#include <api/ui/Interface.h>
#include <api/ui/shell/GameMenu.h>
#include <api/ui/shell/Screen.h>
#include <api/ui/shell/StatisticsOverlay.h>

#include <string>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::render::realtime {
class WindowRenderer;
};  // namespace v3d::render::realtime

namespace v3d::shell {

/**
 * What every app builds around its own code: the ui and its document, the screen its text and
 * ui are drawn on, the pause menu, and the statistics overlay with the command that shows it.
 *
 * An app holds one, built in its constructor, and makes three calls on it:
 *
 * - The constructor declares the commands, so it runs in the app's constructor, before the
 *   engine reads the bindings that send them.
 * - start(), in the app's start(), builds the ui from the "ui" config document and the menu.
 * - present(), once the app's renderer exists, builds the screen over it.
 *
 * release(), in the app's release(), drops what draws through the renderer, so it runs before
 * the renderer is released. Each part is an option, and a part not asked for is not built.
 *
 * The shell holds the address of its own members in the commands it declares, so it cannot be
 * copied or moved.
 **/
class Shell final {
 public:
    /**
     * Which parts of the shell an app has, and the theme it is drawn in.
     **/
    struct Options final {
        /**
         * Put up the menu the ui document names under GameMenu's default names, which pauses
         * the game through the callback start() is given.
         **/
        bool menu { true };
        /** The command that shows and hides the statistics overlay, or empty for none. **/
        std::string statistics;
        /**
         * The document whose themes go beneath the ui document's own, so the ui document names
         * only what it changes. Defaults to the game theme, and empty puts nothing beneath.
         **/
        std::string theme { gameTheme };
    };

    /**
     * The theme every game's shell is drawn in, which a game's own theme is put over.
     **/
    static const char* const gameTheme;

    /**
     * The screen a game's shell is drawn on: body text 26 pixels high in a window 1080 high,
     * scaled with the window's height.
     **/
    static v3d::ui::shell::Screen::Options game();

    /**
     * @param engine the app the shell serves, which outlives it
     * @param options copied. The constructor declares the menu's commands and the statistics
     *        command
     **/
    Shell(v3d::engine::Engine* engine, const Options& options);

    Shell(const Shell&) = delete;
    Shell& operator=(const Shell&) = delete;
    Shell(Shell&&) = delete;
    Shell& operator=(Shell&&) = delete;
    ~Shell() = default;

    /**
     * Build the ui, load the "ui" config document into it with Options::theme beneath it, and
     * build the menu. An app with no ui document has a ui with nothing in it but that theme.
     *
     * @param suspend what the menu does to the game as it opens and closes. Unused without the
     *        menu
     * @return false when the ui document or the theme beneath it does not load, which is
     *         logged
     **/
    bool start(const v3d::ui::shell::GameMenu::Suspend& suspend = v3d::ui::shell::GameMenu::Suspend());

    /**
     * Build the screen over the app's renderer, which outlives it until release().
     *
     * @param renderer the app's renderer, which the screen draws through
     * @param options how the screen draws text, and which of its parts it builds
     **/
    void present(v3d::render::realtime::WindowRenderer* renderer, const v3d::ui::shell::Screen::Options& options);

    /**
     * Drop the screen, the menu and the ui. Called before the renderer is released.
     **/
    void release();

    /**
     * @return the ui, or null before start()
     **/
    const boost::shared_ptr<v3d::ui::Interface>& ui() const noexcept;

    /**
     * @return the menu, or null before start() and without Options::menu
     **/
    const boost::shared_ptr<v3d::ui::shell::GameMenu>& menu() const noexcept;

    /**
     * @return the screen, or null before present()
     **/
    v3d::ui::shell::Screen* screen() noexcept;

    /**
     * What the loop measured, for the statistics overlay: the frame's times and steps, and a
     * span for each scope the app measured.
     **/
    v3d::ui::shell::StatisticsOverlay::Sample sample() const;

    /**
     * Draw the ui, and the statistics over it, into the screen's canvas.
     *
     * @param sample what the overlay shows. An app that adds spans of its own, such as the
     *        device's pass times, adds them to sample() and passes the result
     **/
    void draw(const v3d::ui::shell::StatisticsOverlay::Sample& sample);
    void draw();

 private:
    /**
     * Give the screen the monitor's new scale.
     **/
    void rescale(const v3d::event::kind::DisplayScale& event);

    /**
     * The height of a game's body text in a window Screen::referenceHeight high.
     **/
    static constexpr float bodySize = 26.0F;

    v3d::engine::Engine* engine_;
    Options options_;
    boost::shared_ptr<v3d::ui::Interface> ui_;
    boost::shared_ptr<v3d::ui::shell::GameMenu> menu_;
    boost::shared_ptr<v3d::ui::shell::Screen> screen_;
    // connected while there is a screen. A shell does not outlive its engine's dispatcher
    entt::scoped_connection scaled_;
};

};  // namespace v3d::shell
