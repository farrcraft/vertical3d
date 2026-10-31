/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Resolver.h"

#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Palette.h>
#include <api/ui/style/Style.h>

#include <algorithm>
#include <cstddef>
#include <functional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

#include <glm/vec4.hpp>

#include "Theme.h"

namespace v3d::ui::style {

const char* const Resolver::tools = "tools";
const char* const Resolver::chromeClass = "ui";

boost::shared_ptr<Style> lookup(const boost::shared_ptr<Theme>& theme,
    const std::string& className, const std::string_view& name) {
    if (!theme) {
        return nullptr;
    }
    const std::vector<boost::shared_ptr<Style>> styles =
        theme->getStyleSet(std::string(name), className);
    return styles.empty() ? nullptr : styles.front();
}

Resolver::Resolver() :
scale_(1.0F) {
}

void Resolver::scale(float factor) noexcept {
    scale_ = factor;
}

float Resolver::scale() const noexcept {
    return scale_;
}

void Resolver::theme(const boost::shared_ptr<Theme>& theme) {
    // the new theme starts from what was there before any theme, so where it is silent the
    // last theme's value does not linger
    overlay_.restore(&base_);
    theme_ = theme;
    for (std::map<std::string, paint::Dressing, std::less<>>& entries : resolved_) {
        entries.clear();
    }
    chrome();
}

boost::shared_ptr<Theme> Resolver::theme() const noexcept {
    return theme_;
}

paint::Dressing& Resolver::base() noexcept {
    // the caller is about to write what every answer was worked out from
    for (std::map<std::string, paint::Dressing, std::less<>>& entries : resolved_) {
        entries.clear();
    }
    return base_;
}

const paint::Dressing& Resolver::base() const noexcept {
    return base_;
}

void readBase(const boost::shared_ptr<Style>& style, paint::Palette::Use use, Overlay<paint::Dressing>* overlay,
    paint::Dressing* dressing, float scale) {
    if (!style) {
        return;
    }

    if (style->property("base", "color") || style->property("accent", "color") ||
        style->property("contrast", "number")) {
        glm::vec4 base = paint::Palette::base;
        glm::vec4 accent = paint::Palette::accent;
        float contrast = 1.0F;
        readColour(style, "base", &base);
        readColour(style, "accent", &accent);
        readMetric(style, "contrast", &contrast);
        for (const paint::Palette::Role& role : paint::Palette(base, accent, contrast).roles(use)) {
            overlay->set(dressing, role.field, role.colour);
        }
    }

    overlay->colour(style, "gap", dressing, &paint::Dressing::gap);
    overlay->colour(style, "bar", dressing, &paint::Dressing::bar);
    overlay->colour(style, "panel", dressing, &paint::Dressing::panel);
    overlay->colour(style, "border", dressing, &paint::Dressing::border);
    overlay->colour(style, "title-bar", dressing, &paint::Dressing::titleBar);
    overlay->colour(style, "track", dressing, &paint::Dressing::track);
    overlay->colour(style, "fill", dressing, &paint::Dressing::fill);
    overlay->colour(style, "thumb", dressing, &paint::Dressing::thumb);
    overlay->colour(style, "mark", dressing, &paint::Dressing::mark);
    overlay->colour(style, "caret", dressing, &paint::Dressing::caret);
    overlay->colour(style, "placeholder", dressing, &paint::Dressing::placeholder);
    overlay->colour(style, "tab", dressing, &paint::Dressing::tab);
    overlay->colour(style, "widget", dressing, &paint::Dressing::widget);
    overlay->colour(style, "rule", dressing, &paint::Dressing::rule);
    overlay->colour(style, "hairline", dressing, &paint::Dressing::hairline);
    overlay->colour(style, "text", dressing, &paint::Dressing::text);
    overlay->colour(style, "active-text", dressing, &paint::Dressing::activeText);
    overlay->colour(style, "secondary-text", dressing, &paint::Dressing::secondaryText);
    overlay->colour(style, "disabled-text", dressing, &paint::Dressing::disabledText);
    overlay->colour(style, "highlight", dressing, &paint::Dressing::highlight);
    overlay->colour(style, "selected", dressing, &paint::Dressing::selected);
    overlay->colour(style, "hover", dressing, &paint::Dressing::hover);
    overlay->colour(style, "pressed", dressing, &paint::Dressing::pressed);
    overlay->colour(style, "focus", dressing, &paint::Dressing::focus);
    overlay->colour(style, "shadow", dressing, &paint::Dressing::shadow);
    overlay->colour(style, "backdrop", dressing, &paint::Dressing::backdrop);
    // over whatever the panel came to, derived or named, so a translucent panel follows the base
    if (style->property("panel-opacity", "number")) {
        float opacity = 1.0F;
        readMetric(style, "panel-opacity", &opacity);
        glm::vec4 panel = dressing->panel;
        panel.a *= std::clamp(opacity, 0.0F, 1.0F);
        overlay->set(dressing, &paint::Dressing::panel, panel);
    }

    overlay->metric(style, "line-height", dressing, &paint::Dressing::lineHeight, scale);
    overlay->metric(style, "padding", dressing, &paint::Dressing::padding, scale);
    overlay->metric(style, "spacing", dressing, &paint::Dressing::spacing, scale);
    overlay->metric(style, "bar-height", dressing, &paint::Dressing::barHeight, scale);
    overlay->metric(style, "icon-size", dressing, &paint::Dressing::iconSize, scale);
    overlay->metric(style, "panel-padding", dressing, &paint::Dressing::panelPadding, scale);
    overlay->metric(style, "scrollbar-width", dressing, &paint::Dressing::scrollbarWidth, scale);
    overlay->metric(style, "mark-size", dressing, &paint::Dressing::markSize, scale);
    overlay->width(style, "border-width", dressing, &paint::Dressing::borderWidth, scale, &paint::Dressing::outline);
    overlay->width(style, "focus-width", dressing, &paint::Dressing::focusWidth, scale, &paint::Dressing::outline);
    overlay->metric(style, "radius", dressing, &paint::Dressing::radius, scale);
    overlay->metric(style, "indent", dressing, &paint::Dressing::indent, scale);
    overlay->metric(style, "shadow-size", dressing, &paint::Dressing::shadowSize, scale);
    overlay->metric(style, "shadow-offset", dressing, &paint::Dressing::shadowOffset, scale);
    overlay->metric(style, "open-distance", dressing, &paint::Dressing::openDistance, scale);
    overlay->metric(style, "hover-duration", dressing, &paint::Dressing::hoverDuration);
    overlay->metric(style, "open-duration", dressing, &paint::Dressing::openDuration);
    overlay->metric(style, "submenu-delay", dressing, &paint::Dressing::submenuDelay);
    overlay->metric(style, "tooltip-delay", dressing, &paint::Dressing::tooltipDelay);
    overlay->metric(style, "tooltip-quick-delay", dressing, &paint::Dressing::tooltipQuickDelay);
    overlay->metric(style, "title-weight", dressing, &paint::Dressing::titleWeight);
}

void Resolver::chrome() {
    readBase(lookup(chromeClass, std::string_view()), paint::Palette::Use::Chrome, &overlay_, &base_, scale_);
}

const char* Resolver::named(Class className) noexcept {
    switch (className) {
        case Class::Panel:     return "panel";
        case Class::Bar:       return "bar";
        case Class::Scrollbar: return "scrollbar";
        case Class::Slider:    return "slider";
        case Class::CheckBox:  return "checkbox";
        case Class::Radio:     return "radio";
        case Class::List:      return "list";
        case Class::Tabs:      return "tabs";
        case Class::TextBox:   return "textbox";
        case Class::Button:    return "button";
    }
    return "";
}

boost::shared_ptr<Style> Resolver::lookup(const std::string& className,
    const std::string_view& name) const {
    return style::lookup(theme_, className, name);
}

paint::Dressing Resolver::dress(Class className, const std::string_view& name) const {
    paint::Dressing dressing = base_;
    const boost::shared_ptr<Style> style = lookup(named(className), name);
    if (!style) {
        return dressing;
    }
    readClass(className, style, &dressing);
    // every class may override the focus ring, so it is read once here for all of them.
    // A class naming neither property draws the base's ring
    readColour(style, "focus", &dressing.focus);
    width(style, "focus-width", &dressing.focusWidth);
    return dressing;
}

void Resolver::metric(const boost::shared_ptr<Style>& from, const std::string& key, float* into) const {
    if (from->property(key, "number")) {
        float read = 0.0F;
        readMetric(from, key, &read);
        *into = read * scale_;
    }
}

void Resolver::width(const boost::shared_ptr<Style>& from, const std::string& key, float* into) const {
    if (from->property(key, "number")) {
        float read = 0.0F;
        readMetric(from, key, &read);
        *into = paint::Dressing::outline(read, scale_);
    }
}

void Resolver::readClass(Class className, const boost::shared_ptr<Style>& style, paint::Dressing* into) const {
    paint::Dressing& dressing = *into;
    switch (className) {
        case Class::Panel:
            readColour(style, "background", &dressing.panel);
            readColour(style, "border", &dressing.border);
            width(style, "border-width", &dressing.borderWidth);
            metric(style, "radius", &dressing.radius);
            break;
        case Class::Bar:
            readColour(style, "track", &dressing.track);
            readColour(style, "fill", &dressing.fill);
            readColour(style, "border", &dressing.border);
            width(style, "border-width", &dressing.borderWidth);
            metric(style, "radius", &dressing.radius);
            break;
        case Class::Scrollbar:
            // a scrollbar dresses from its own class, so a theme that paints a health bar
            // green does not also paint every scrollbar green
            readColour(style, "track", &dressing.track);
            readColour(style, "thumb", &dressing.thumb);
            readColour(style, "border", &dressing.border);
            width(style, "border-width", &dressing.borderWidth);
            metric(style, "radius", &dressing.radius);
            break;
        case Class::Slider:
            // a track, the fill up to the thumb, and the thumb. A class of its own, so a
            // theme's bar and scrollbar styles do not change it
            readColour(style, "track", &dressing.track);
            readColour(style, "fill", &dressing.fill);
            readColour(style, "thumb", &dressing.thumb);
            readColour(style, "border", &dressing.border);
            width(style, "border-width", &dressing.borderWidth);
            metric(style, "radius", &dressing.radius);
            metric(style, "mark-size", &dressing.markSize);
            break;
        case Class::CheckBox:
        case Class::Radio:
            // the box a mark sits in is dressed as a track, because the mark is drawn over
            // it the way a bar's fill is drawn over its track
            readColour(style, "background", &dressing.track);
            readColour(style, "mark", &dressing.mark);
            readColour(style, "border", &dressing.border);
            readColour(style, "text", &dressing.text);
            width(style, "border-width", &dressing.borderWidth);
            metric(style, "mark-size", &dressing.markSize);
            break;
        case Class::List:
            readColour(style, "background", &dressing.panel);
            readColour(style, "border", &dressing.border);
            readColour(style, "highlight", &dressing.highlight);
            readColour(style, "text", &dressing.text);
            readColour(style, "active-text", &dressing.activeText);
            width(style, "border-width", &dressing.borderWidth);
            metric(style, "radius", &dressing.radius);
            metric(style, "line-height", &dressing.lineHeight);
            break;
        case Class::Tabs:
            readColour(style, "background", &dressing.panel);
            readColour(style, "tab", &dressing.tab);
            readColour(style, "highlight", &dressing.highlight);
            readColour(style, "text", &dressing.text);
            readColour(style, "active-text", &dressing.activeText);
            readColour(style, "border", &dressing.border);
            metric(style, "bar-height", &dressing.barHeight);
            metric(style, "radius", &dressing.radius);
            break;
        case Class::TextBox:
            readColour(style, "background", &dressing.panel);
            readColour(style, "border", &dressing.border);
            readColour(style, "text", &dressing.text);
            readColour(style, "caret", &dressing.caret);
            readColour(style, "placeholder", &dressing.placeholder);
            // what is drawn behind the selected run, the way a list highlights a chosen row
            readColour(style, "highlight", &dressing.highlight);
            width(style, "border-width", &dressing.borderWidth);
            metric(style, "radius", &dressing.radius);
            metric(style, "line-height", &dressing.lineHeight);
            break;
        case Class::Button:
            // a button's fill comes from its skin and its label colour from the base, so
            // the ring dress() reads is all it reads as a Dressing
            break;
    }
}

const paint::Dressing& Resolver::resolve(Class className, const std::string_view& name) const {
    std::map<std::string, paint::Dressing, std::less<>>& entries =
        resolved_[static_cast<std::size_t>(className)];
    const auto found = entries.find(name);
    if (found != entries.end()) {
        return found->second;
    }
    return entries.emplace(name, dress(className, name)).first->second;
}

};  // namespace v3d::ui::style
