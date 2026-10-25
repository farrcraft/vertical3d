/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/ui/Container.h>
#include <api/ui/Engine.h>
#include <api/ui/Image.h>
#include <vertical3d/src/scene/RenderSettings.h>

#include <optional>
#include <string>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor {

/**
 * The render window and the render settings, in a ui the editor's config lays out.
 *
 * Like the file chooser, it drives components the config names rather than being one. The render
 * window is the "render-view" container: a label for the render's state, an icon for its
 * picture, and buttons. The settings are the "render-settings" container: a text box each for
 * the width, the height and the samples, and a list of the two hiders. With a config that names
 * none of them, every call does nothing and settings() has nothing to read.
 **/
class RenderPanels final {
 public:
    explicit RenderPanels(const boost::shared_ptr<v3d::ui::Engine>& engine);

    void showView();
    void hideView();
    bool viewVisible() const;

    /**
     * Say what the render is doing, as the window's first line.
     **/
    void status(const std::string& text);

    /**
     * Show a picture, fitted within the room the config gives it without changing its shape.
     **/
    void picture(const v3d::ui::Image& image, const glm::uvec2& size);

    /**
     * Put the settings up, with their fields filled from the settings given.
     **/
    void openSettings(const RenderSettings& settings);
    void closeSettings();
    bool settingsVisible() const;

    /**
     * @return the settings the fields hold, or nothing when a field is not a whole number or
     *         the settings it makes are not valid
     **/
    std::optional<RenderSettings> settings() const;

 private:
    boost::shared_ptr<v3d::ui::Container> container(const char* name) const;

    boost::shared_ptr<v3d::ui::Engine> engine_;
    // the room the config gives the picture, read when the first picture is shown
    glm::vec2 room_{ 0.0F };
};

};  // namespace v3d::editor
