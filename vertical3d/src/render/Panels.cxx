/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Panels.h"

#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Image.h>
#include <api/ui/Length.h>
#include <api/ui/component/Icon.h>
#include <api/ui/component/Label.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/TextBox.h>
#include <vertical3d/src/scene/RenderSettings.h>

#include <algorithm>
#include <charconv>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>

namespace v3d::editor::render {

/**
 **/
template <typename T>
boost::shared_ptr<T> Panels::find(const boost::shared_ptr<v3d::ui::Container>& container, const char* name) {
    return container ? boost::dynamic_pointer_cast<T>(container->get(name)) : boost::shared_ptr<T>();
}

/**
 **/
std::optional<unsigned int> Panels::number(const boost::shared_ptr<v3d::ui::Container>& container, const char* name) {
    const boost::shared_ptr<v3d::ui::component::TextBox> field = find<v3d::ui::component::TextBox>(container, name);
    if (!field) {
        return std::nullopt;
    }
    const std::string_view text = field->text();
    unsigned int value = 0;
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc() || end != text.data() + text.size()) {
        return std::nullopt;
    }
    return value;
}

/**
 **/
Panels::Panels(const boost::shared_ptr<v3d::ui::Interface>& engine) :
    engine_(engine) {
}

/**
 **/
void Panels::showView() {
    const boost::shared_ptr<v3d::ui::Container> view = container(VIEW);
    if (view) {
        view->visible(true);
    }
}

/**
 **/
void Panels::hideView() {
    const boost::shared_ptr<v3d::ui::Container> view = container(VIEW);
    if (view) {
        view->visible(false);
    }
}

/**
 **/
bool Panels::viewVisible() const {
    const boost::shared_ptr<v3d::ui::Container> view = container(VIEW);
    return view && view->visible();
}

/**
 **/
void Panels::status(const std::string& text) {
    const boost::shared_ptr<v3d::ui::component::Label> label =
        find<v3d::ui::component::Label>(container(VIEW), "renderStatus");
    if (label) {
        label->text(text);
    }
}

/**
 **/
void Panels::picture(const v3d::ui::Image& image, const glm::uvec2& size) {
    const boost::shared_ptr<v3d::ui::component::Icon> icon =
        find<v3d::ui::component::Icon>(container(VIEW), "renderPicture");
    if (!icon || size.x == 0 || size.y == 0) {
        return;
    }
    // the config's size is the room the picture has, and the picture is scaled to fit it
    if (room_.x <= 0.0F) {
        room_ = glm::vec2(icon->layout().width.value(), icon->layout().height.value());
    }
    const float scale = std::min(room_.x / static_cast<float>(size.x), room_.y / static_cast<float>(size.y));
    icon->layout().width = v3d::ui::Length(static_cast<float>(size.x) * scale, v3d::ui::Length::Unit::Pixels);
    icon->layout().height = v3d::ui::Length(static_cast<float>(size.y) * scale, v3d::ui::Length::Unit::Pixels);
    icon->image(image);
}

/**
 **/
void Panels::openSettings(const scene::RenderSettings& settings) {
    const boost::shared_ptr<v3d::ui::Container> panel = container(SETTINGS);
    if (!panel) {
        return;
    }
    const auto fill = [&panel](const char* name, unsigned int value) {
        const boost::shared_ptr<v3d::ui::component::TextBox> field = find<v3d::ui::component::TextBox>(panel, name);
        if (field) {
            field->text(std::to_string(value));
        }
    };
    fill("settingsWidth", settings.size.x);
    fill("settingsHeight", settings.size.y);
    fill("settingsSamples", settings.samples);
    const boost::shared_ptr<v3d::ui::component::SelectList> hider =
        find<v3d::ui::component::SelectList>(panel, "settingsHider");
    if (hider) {
        hider->selected(settings.hider == scene::RenderSettings::Hider::RayTrace ? 1 : 0);
    }
    panel->visible(true);
}

/**
 **/
void Panels::closeSettings() {
    const boost::shared_ptr<v3d::ui::Container> panel = container(SETTINGS);
    if (panel) {
        panel->visible(false);
    }
}

/**
 **/
bool Panels::settingsVisible() const {
    const boost::shared_ptr<v3d::ui::Container> panel = container(SETTINGS);
    return panel && panel->visible();
}

/**
 **/
std::optional<scene::RenderSettings> Panels::settings() const {
    const boost::shared_ptr<v3d::ui::Container> panel = container(SETTINGS);
    const std::optional<unsigned int> width = number(panel, "settingsWidth");
    const std::optional<unsigned int> height = number(panel, "settingsHeight");
    const std::optional<unsigned int> samples = number(panel, "settingsSamples");
    const boost::shared_ptr<v3d::ui::component::SelectList> hider =
        find<v3d::ui::component::SelectList>(panel, "settingsHider");
    if (!width || !height || !samples) {
        return std::nullopt;
    }
    scene::RenderSettings settings;
    settings.size = glm::uvec2(*width, *height);
    settings.samples = *samples;
    settings.hider = hider && hider->selected() == 1 ? scene::RenderSettings::Hider::RayTrace : scene::RenderSettings::Hider::Hidden;
    if (!settings.valid()) {
        return std::nullopt;
    }
    return settings;
}

/**
 **/
boost::shared_ptr<v3d::ui::Container> Panels::container(const char* name) const {
    return engine_ ? engine_->container(name) : boost::shared_ptr<v3d::ui::Container>();
}

};  // namespace v3d::editor::render
