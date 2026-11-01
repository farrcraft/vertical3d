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

#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/pointer_cast.hpp>
#include <boost/shared_ptr.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::control {

/**
 **/
TopBar::TopBar(const boost::shared_ptr<v3d::ui::Interface>& ui, const std::string& before, const v3d::ui::paint::Measure& measure) :
    ui_(ui),
    before_(before),
    measure_(measure) {
}

/**
 **/
std::string TopBar::title(const std::string& project, bool dirty) {
    return dirty ? project + UNSAVED : project;
}

/**
 **/
boost::json::object TopBar::document(const std::vector<std::string>& workspaces) {
    boost::json::array tabs;
    for (std::size_t index = 0; index < workspaces.size(); index++) {
        tabs.emplace_back(boost::json::object{ { "type", "button" }, { "name", "workspace-" + std::to_string(index) },
            { "label", workspaces[index] }, { "toggle", true }, { "context", "workspace" }, { "command", "show" },
            { "data", workspaces[index] } });
    }
    boost::json::array components = {
        boost::json::object{ { "type", "hbox" }, { "name", "workspaces" }, { "children", std::move(tabs) } },
        boost::json::object{ { "type", "label" }, { "name", "project" }, { "label", "" } },
        boost::json::object{ { "type", "button" }, { "name", "search" }, { "label", SEARCH }, { "tooltip", "Search the commands" },
            { "context", "search" }, { "command", "open" } },
    };
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
    }
    const boost::shared_ptr<v3d::ui::Container> container = ui_->container(std::string(NAME));
    if (!container) {
        return;
    }
    const auto width = [this](const std::string& text) { return measure_ ? measure_(text) : 0.0F; };
    // a gap of half the bar's height either side of each piece of text
    const float pad = band.w / 2.0F;

    const float search = width(SEARCH) + (pad * 2.0F);
    const float right = band.x + band.z;
    if (const boost::shared_ptr<v3d::ui::Component> button = container->get("search")) {
        box(button.get(), glm::vec4(right - search, band.y, search, band.w));
    }
    const std::string named = title(state.project, state.dirty);
    const float name = width(named) + (pad * 2.0F);
    const boost::shared_ptr<v3d::ui::component::Label> label = boost::dynamic_pointer_cast<v3d::ui::component::Label>(container->get("project"));
    if (label) {
        label->text(named);
        box(label.get(), glm::vec4(right - search - name, band.y, name, band.w));
    }
    if (const boost::shared_ptr<v3d::ui::Component> tabs = container->get("workspaces")) {
        box(tabs.get(), glm::vec4(band.x + pad, band.y, std::max(0.0F, band.z - search - name - (pad * 2.0F)), band.w));
        for (std::size_t index = 0; index < state.workspaces.size(); index++) {
            const boost::shared_ptr<v3d::ui::component::Button> tab =
                boost::dynamic_pointer_cast<v3d::ui::component::Button>(container->get("workspace-" + std::to_string(index)));
            if (tab) {
                tab->checked(state.workspaces[index] == state.shown);
            }
        }
    }
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

};  // namespace v3d::editor::control
