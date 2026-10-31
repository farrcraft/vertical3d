/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Theme.h"

#include "Button.h"
#include "Menu.h"
#include "Style.h"

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::ui::style {

namespace {

/**
 * Whether two styles dress the same thing: the same class and name, and for a button the same
 * state.
 **/
bool alike(const Style& one, const Style& other) {
    if (one.className() != other.className() || one.name() != other.name()) {
        return false;
    }
    const auto* button = dynamic_cast<const Button*>(&one);
    const auto* otherButton = dynamic_cast<const Button*>(&other);
    if (button == nullptr || otherButton == nullptr) {
        return button == otherButton;
    }
    return button->state() == otherButton->state();
}

/**
 * A new style of the same kind, class and name as another, holding the same properties. The
 * properties are shared; the style is not, so merging into one theme's copy leaves another's.
 **/
boost::shared_ptr<Style> copied(const Style& style) {
    boost::shared_ptr<Style> made;
    if (const auto* button = dynamic_cast<const Button*>(&style)) {
        made = boost::make_shared<Button>(std::string(style.name()), button->state());
    } else if (dynamic_cast<const Menu*>(&style) != nullptr) {
        made = boost::make_shared<Menu>(std::string(style.name()));
    } else {
        made = boost::make_shared<Style>(Selector{ .name = std::string(style.name()), .className = std::string(style.className()) });
    }
    made->under(style);
    return made;
}

};  // namespace

Theme::Theme(const std::string& str) : name_(str) {
}

Theme::~Theme() {
}

std::string_view Theme::name() const {
    return name_;
}

void Theme::addStyle(const boost::shared_ptr<Style>& style) {
    styles_.push_back(style);
}

std::vector< boost::shared_ptr<Style> > Theme::getStyleSet(const std::string& name, const std::string& class_name) const {
    std::vector< boost::shared_ptr<Style> > matching_styles;
    std::vector< boost::shared_ptr<Style> >::const_iterator iter = styles_.begin();
    for (; iter != styles_.end(); iter++) {
        if ((name.empty() || (*iter)->name() == name) && (class_name.empty() || (*iter)->className() == class_name)) {
            matching_styles.push_back((*iter));
        }
    }
    return matching_styles;
}

void Theme::beneath(const Theme& lower) {
    // only the styles this theme held before, so a style added from the other is not matched
    // against the rest of the other's
    const std::size_t own = styles_.size();
    for (const boost::shared_ptr<Style>& under : lower.styles_) {
        bool matched = false;
        for (std::size_t index = 0; index < own; index++) {
            if (alike(*styles_[index], *under)) {
                styles_[index]->under(*under);
                matched = true;
                break;
            }
        }
        if (!matched) {
            styles_.push_back(copied(*under));
        }
    }
}

};  // namespace v3d::ui::style
