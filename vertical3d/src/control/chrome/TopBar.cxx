/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "TopBar.h"

#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/Layout.h>
#include <api/ui/Length.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Label.h>
#include <api/ui/paint/Text.h>
#include <api/ui/paint/Type.h>

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control::chrome {

/**
 **/
TopBar::TopBar(const boost::shared_ptr<v3d::ui::Interface>& ui, const std::string& before, const v3d::ui::paint::Type& type,
    const Resolve& resolve) :
    ui_(ui),
    before_(before),
    type_(type),
    resolve_(resolve) {
}

/**
 **/
boost::json::object TopBar::document(const std::vector<std::string>& workspaces) {
    // the tabs lie in one recess, drawn first so that it is under them
    boost::json::array components = {
        boost::json::object{ { "type", "panel" }, { "name", "workspaces" }, { "recess", true } },
    };
    for (std::size_t index = 0; index < workspaces.size(); index++) {
        components.emplace_back(boost::json::object{ { "type", "button" }, { "name", "workspace-" + std::to_string(index) },
            { "label", workspaces[index] }, { "toggle", true }, { "tab", "choice" }, { "context", "workspace" },
            { "command", "show" }, { "data", workspaces[index] } });
    }
    components.emplace_back(boost::json::object{ { "type", "label" }, { "name", "project" }, { "label", "" } });
    components.emplace_back(boost::json::object{ { "type", "icon" }, { "name", "unsaved" }, { "source", "icon:unsaved" } });
    // the search's plate, then the button that takes the press over all of it, then what it
    // shows, none of which takes a press
    components.emplace_back(boost::json::object{ { "type", "panel" }, { "name", "search-plate" } });
    components.emplace_back(boost::json::object{ { "type", "button" }, { "name", "search" }, { "label", "" },
        { "tooltip", "Search the commands" }, { "context", "search" }, { "command", "open" } });
    components.emplace_back(boost::json::object{ { "type", "icon" }, { "name", "search-icon" }, { "source", "icon:search" } });
    components.emplace_back(boost::json::object{ { "type", "label" }, { "name", "search-label" }, { "label", SEARCH } });
    components.emplace_back(boost::json::object{ { "type", "panel" }, { "name", "search-cap" }, { "recess", true } });
    components.emplace_back(boost::json::object{ { "type", "label" }, { "name", "search-key" }, { "label", "" } });
    boost::json::array containers = {
        boost::json::object{ { "name", NAME }, { "visible", true }, { "components", std::move(components) } },
    };
    return boost::json::object{ { "containers", std::move(containers) } };
}

/**
 **/
void TopBar::sync(const State& state, const glm::vec4& band) {
    if (!added_ || state.workspaces != built_) {
        if (added_) {
            ui_->remove(std::string(NAME));
        }
        added_ = ui_->add(document(state.workspaces), before_);
        built_ = state.workspaces;
        // the icons are named, and resolved to images once the container holding them exists
        const boost::shared_ptr<v3d::ui::Container> made = added_ ? ui_->container(std::string(NAME)) : nullptr;
        if (made && resolve_) {
            for (const boost::shared_ptr<v3d::ui::Component>& component : made->components()) {
                resolve_(component);
            }
        }
    }
    const boost::shared_ptr<v3d::ui::Container> container = ui_->container(std::string(NAME));
    if (!container) {
        return;
    }
    const auto width = [this](const std::string& text) { return type_.width(text); };
    // a gap of half the bar's height either side of each piece of text
    const float pad = band.w / 2.0F;
    const float inset = band.w * TAB_INSET;
    const float tall = band.w - (inset * 2.0F);

    // the tabs sit side by side in their recess, each its label and a gap either side, inset
    // from the bar's top and foot. A label is measured semibold, as the chosen tab is set, so
    // showing another workspace moves none of them
    const v3d::ui::paint::Type strong = type_.in(v3d::ui::paint::Weight::SemiBold);
    const float start = band.x + pad;
    float at = start + inset;
    for (std::size_t index = 0; index < state.workspaces.size(); index++) {
        const boost::shared_ptr<v3d::ui::component::Button> tab =
            boost::dynamic_pointer_cast<v3d::ui::component::Button>(container->get("workspace-" + std::to_string(index)));
        if (tab) {
            const float wide = strong.width(state.workspaces[index]) + pad;
            box(tab.get(), glm::vec4(at, band.y + inset, wide, tall));
            tab->checked(state.workspaces[index] == state.shown);
            at += wide;
        }
    }
    const float tabsEnd = at + inset;
    if (const boost::shared_ptr<v3d::ui::Component> plate = container->get("workspaces")) {
        box(plate.get(), glm::vec4(start, band.y + (inset / 2.0F), tabsEnd - start, band.w - inset));
    }

    // the name ends a gap before the search, with the dot after it while there are unsaved
    // changes. The search leaves its label out when the name would otherwise reach the tabs. A
    // label draws its text from its box's left edge
    const float side = tall * DOT_SHARE;
    const float name = width(state.project);
    const float named = name + (state.dirty ? side + (pad / 2.0F) : 0.0F);
    float end = placeSearch(container.get(), state, band, tabsEnd + pad + named + pad) - pad;
    if (const boost::shared_ptr<v3d::ui::Component> dot = container->get("unsaved")) {
        dot->visible(state.dirty);
        if (state.dirty) {
            box(dot.get(), glm::vec4(end - side, band.y + inset + ((tall - side) / 2.0F), side, side));
            end -= side + (pad / 2.0F);
        }
    }
    const boost::shared_ptr<v3d::ui::component::Label> label = boost::dynamic_pointer_cast<v3d::ui::component::Label>(container->get("project"));
    if (label) {
        label->text(state.project);
        box(label.get(), glm::vec4(end - name, band.y, name, band.w));
    }
}

/**
 **/
float TopBar::placeSearch(v3d::ui::Container* container, const State& state, const glm::vec4& band, float limit) const {
    const auto width = [this](const std::string& text) { return type_.width(text); };
    const float gap = band.w / 4.0F;
    const float inset = band.w * TAB_INSET;
    const float top = band.y + inset;
    const float tall = band.w - (inset * 2.0F);
    const float icon = tall * ICON_SHARE;
    // the plate holds a gap, the magnifier, a gap, what it does and then the key on its cap
    // with a gap after it. With no key bound there is no cap. When the plate would start before
    // the limit it leaves out what it does, and holds the magnifier and the key alone
    const bool keyed = !state.searchKey.empty();
    const float cap = keyed ? width(state.searchKey) + gap : 0.0F;
    const float bare = (gap * 2.0F) + icon + (keyed ? cap + gap : 0.0F);
    const float right = band.x + band.z - gap;
    const bool labelled = right - (bare + gap + width(SEARCH)) >= limit;
    const float wide = labelled ? bare + gap + width(SEARCH) : bare;
    const float left = right - wide;

    for (const char* name : { "search-plate", "search" }) {
        if (const boost::shared_ptr<v3d::ui::Component> part = container->get(name)) {
            box(part.get(), glm::vec4(left, top, wide, tall));
        }
    }
    if (const boost::shared_ptr<v3d::ui::Component> magnifier = container->get("search-icon")) {
        box(magnifier.get(), glm::vec4(left + gap, top + ((tall - icon) / 2.0F), icon, icon));
    }
    if (const boost::shared_ptr<v3d::ui::Component> label = container->get("search-label")) {
        label->visible(labelled);
        box(label.get(), glm::vec4(left + (gap * 2.0F) + icon, band.y, width(SEARCH), band.w));
    }
    const float capLeft = left + wide - gap - cap;
    if (const boost::shared_ptr<v3d::ui::Component> plate = container->get("search-cap")) {
        const float high = tall * CAP_SHARE;
        plate->visible(keyed);
        box(plate.get(), glm::vec4(capLeft, top + ((tall - high) / 2.0F), cap, high));
    }
    const boost::shared_ptr<v3d::ui::component::Label> key = boost::dynamic_pointer_cast<v3d::ui::component::Label>(container->get("search-key"));
    if (key) {
        key->visible(keyed);
        key->text(state.searchKey);
        box(key.get(), glm::vec4(capLeft + (gap / 2.0F), band.y, width(state.searchKey), band.w));
    }
    return left;
}

/**
 **/
void TopBar::box(v3d::ui::Component* component, const glm::vec4& region) {
    v3d::ui::Layout& layout = component->layout();
    layout.x = v3d::ui::Length(region.x, v3d::ui::Length::Unit::Pixels);
    layout.y = v3d::ui::Length(region.y, v3d::ui::Length::Unit::Pixels);
    layout.width = v3d::ui::Length(std::max(0.0F, region.z), v3d::ui::Length::Unit::Pixels);
    layout.height = v3d::ui::Length(std::max(0.0F, region.w), v3d::ui::Length::Unit::Pixels);
}

};  // namespace v3d::editor::control::chrome
