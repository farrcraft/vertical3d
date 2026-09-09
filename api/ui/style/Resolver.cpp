/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Resolver.h"

#include <api/ui/style/Style.h>

#include <functional>
#include <map>
#include <string>
#include <vector>

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

Resolver::Resolver() {
}

void Resolver::theme(const boost::shared_ptr<Theme>& theme) {
    theme_ = theme;
    for (std::map<std::string, Dressing, std::less<>>& entries : resolved_) {
        entries.clear();
    }
    chrome();
}

boost::shared_ptr<Theme> Resolver::theme() const noexcept {
    return theme_;
}

Dressing& Resolver::base() noexcept {
    // the caller is about to write what every answer was worked out from
    for (std::map<std::string, Dressing, std::less<>>& entries : resolved_) {
        entries.clear();
    }
    return base_;
}

const Dressing& Resolver::base() const noexcept {
    return base_;
}

void Resolver::chrome() {
    const boost::shared_ptr<Style> style = lookup(chromeClass, std::string_view());
    if (!style) {
        return;
    }

    readColour(style, "panel", &base_.panel);
    readColour(style, "border", &base_.border);
    readColour(style, "track", &base_.track);
    readColour(style, "fill", &base_.fill);
    readColour(style, "thumb", &base_.thumb);
    readColour(style, "mark", &base_.mark);
    readColour(style, "text", &base_.text);
    readColour(style, "active-text", &base_.activeText);
    readColour(style, "highlight", &base_.highlight);
    readColour(style, "hover", &base_.hover);
    readColour(style, "focus", &base_.focus);

    readMetric(style, "line-height", &base_.lineHeight);
    readMetric(style, "padding", &base_.padding);
    readMetric(style, "bar-height", &base_.barHeight);
    readMetric(style, "icon-size", &base_.iconSize);
    readMetric(style, "panel-padding", &base_.panelPadding);
    readMetric(style, "scrollbar-width", &base_.scrollbarWidth);
    readMetric(style, "mark-size", &base_.markSize);
    readMetric(style, "border-width", &base_.borderWidth);
    readMetric(style, "focus-width", &base_.focusWidth);
    readMetric(style, "radius", &base_.radius);
}

const char* Resolver::named(Class className) noexcept {
    switch (className) {
        case Class::Panel:     return "panel";
        case Class::Bar:       return "bar";
        case Class::Scrollbar: return "scrollbar";
        case Class::CheckBox:  return "checkbox";
        case Class::Radio:     return "radio";
        case Class::List:      return "list";
        case Class::Tabs:      return "tabs";
        case Class::TextBox:   return "textbox";
    }
    return "";
}

boost::shared_ptr<Style> Resolver::lookup(const std::string& className,
    const std::string_view& name) const {
    return style::lookup(theme_, className, name);
}

Dressing Resolver::dress(Class className, const std::string_view& name) const {
    Dressing dressing = base_;
    const boost::shared_ptr<Style> style = lookup(named(className), name);
    if (!style) {
        return dressing;
    }

    switch (className) {
        case Class::Panel:
            readColour(style, "background", &dressing.panel);
            readColour(style, "border", &dressing.border);
            readMetric(style, "border-width", &dressing.borderWidth);
            readMetric(style, "radius", &dressing.radius);
            break;
        case Class::Bar:
            readColour(style, "track", &dressing.track);
            readColour(style, "fill", &dressing.fill);
            readColour(style, "border", &dressing.border);
            readMetric(style, "border-width", &dressing.borderWidth);
            readMetric(style, "radius", &dressing.radius);
            break;
        case Class::Scrollbar:
            // a scrollbar is not a progress bar: it dresses from its own class, so a theme
            // that paints a health bar green does not paint a scrollbar green as well
            readColour(style, "track", &dressing.track);
            readColour(style, "thumb", &dressing.thumb);
            readColour(style, "border", &dressing.border);
            readMetric(style, "border-width", &dressing.borderWidth);
            readMetric(style, "radius", &dressing.radius);
            break;
        case Class::CheckBox:
        case Class::Radio:
            // the box a mark sits in is a track rather than a panel: it is the thing the
            // mark is drawn over, the way a bar's fill is drawn over one
            readColour(style, "background", &dressing.track);
            readColour(style, "mark", &dressing.mark);
            readColour(style, "border", &dressing.border);
            readColour(style, "text", &dressing.text);
            readMetric(style, "border-width", &dressing.borderWidth);
            readMetric(style, "mark-size", &dressing.markSize);
            break;
        case Class::List:
            readColour(style, "background", &dressing.panel);
            readColour(style, "border", &dressing.border);
            readColour(style, "highlight", &dressing.highlight);
            readColour(style, "text", &dressing.text);
            readColour(style, "active-text", &dressing.activeText);
            readMetric(style, "border-width", &dressing.borderWidth);
            readMetric(style, "radius", &dressing.radius);
            readMetric(style, "line-height", &dressing.lineHeight);
            break;
        case Class::Tabs:
            readColour(style, "background", &dressing.panel);
            readColour(style, "tab", &dressing.track);
            readColour(style, "highlight", &dressing.highlight);
            readColour(style, "text", &dressing.text);
            readColour(style, "active-text", &dressing.activeText);
            readColour(style, "border", &dressing.border);
            readMetric(style, "bar-height", &dressing.barHeight);
            readMetric(style, "radius", &dressing.radius);
            break;
        case Class::TextBox:
            readColour(style, "background", &dressing.panel);
            readColour(style, "border", &dressing.border);
            readColour(style, "text", &dressing.text);
            // the caret is the mark of a text box: the one thing drawn over the text that
            // is the component's own rather than the app's
            readColour(style, "caret", &dressing.mark);
            readColour(style, "placeholder", &dressing.track);
            readMetric(style, "border-width", &dressing.borderWidth);
            readMetric(style, "radius", &dressing.radius);
            readMetric(style, "line-height", &dressing.lineHeight);
            break;
    }
    return dressing;
}

const Dressing& Resolver::resolve(Class className, const std::string_view& name) const {
    std::map<std::string, Dressing, std::less<>>& entries =
        resolved_[static_cast<std::size_t>(className)];
    const auto found = entries.find(name);
    if (found != entries.end()) {
        return found->second;
    }
    return entries.emplace(name, dress(className, name)).first->second;
}

};  // namespace v3d::ui::style
