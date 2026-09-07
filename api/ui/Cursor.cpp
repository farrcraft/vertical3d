/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Cursor.h"

#include <vector>

#include "Component.h"
#include "Container.h"
#include "Engine.h"
#include "component/Bar.h"
#include "component/Button.h"
#include "component/CheckBox.h"
#include "component/Scrollbar.h"
#include "component/SelectList.h"
#include "component/TabBar.h"
#include "component/Toolbar.h"
#include "component/Type.h"
#include "component/menu/Menu.h"
#include "component/menu/MenuBar.h"

namespace v3d::ui {

namespace {

/**
 * The strips of a container, in the order the cursor is offered them - which is the
 * reverse of the order they are drawn, because an open menu drops a panel over a toolbar.
 **/
void strips(const boost::shared_ptr<Container>& container,
    std::vector<boost::shared_ptr<component::MenuBar>>* bars,
    std::vector<boost::shared_ptr<component::Toolbar>>* toolbars) {
    for (const boost::shared_ptr<Component>& component : container->components()) {
        if (!component || !component->visible()) {
            continue;
        }
        if (component->type() == component::Type::MenuBar) {
            bars->push_back(boost::dynamic_pointer_cast<component::MenuBar>(component));
        } else if (component->type() == component::Type::Toolbar) {
            toolbars->push_back(boost::dynamic_pointer_cast<component::Toolbar>(component));
        }
    }
}

};  // namespace

Cursor::Cursor(const boost::shared_ptr<Engine>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher) :
    ui_(ui),
    dispatcher_(dispatcher) {
}

boost::shared_ptr<Component> Cursor::held() const {
    return held_.lock();
}

bool Cursor::motion(const glm::vec2& point) {
    // a press that has not come up goes on being followed wherever the cursor is, which is
    // what drags a thumb off the bar it started on without losing it
    const boost::shared_ptr<Component> holding = held_.lock();
    if (holding) {
        if (holding->type() == component::Type::Scrollbar) {
            boost::dynamic_pointer_cast<component::Scrollbar>(holding)->drag(point);
        }
        return true;
    }

    if (!ui_) {
        return false;
    }
    bool taken = false;
    for (const boost::shared_ptr<Container>& container : ui_->containers()) {
        if (!container || !container->visible()) {
            continue;
        }
        std::vector<boost::shared_ptr<component::MenuBar>> bars;
        std::vector<boost::shared_ptr<component::Toolbar>> toolbars;
        strips(container, &bars, &toolbars);

        for (const boost::shared_ptr<component::MenuBar>& bar : bars) {
            taken = (bar && bar->motion(point)) || taken;
        }
        for (const boost::shared_ptr<component::Toolbar>& bar : toolbars) {
            if (!bar) {
                continue;
            }
            // every strip hears about it either way, so that a button the cursor has left,
            // or that an open panel now covers, stops drawing its hover
            if (taken) {
                bar->leave();
            } else {
                taken = bar->motion(point) || taken;
            }
        }
        if (!taken && container->pick(point)) {
            taken = true;
        }
    }
    return taken;
}

bool Cursor::press(const glm::vec2& point) {
    if (!ui_) {
        return false;
    }
    for (const boost::shared_ptr<Container>& container : ui_->containers()) {
        if (container && container->visible() && press(container, point)) {
            return true;
        }
    }
    return false;
}

bool Cursor::press(const boost::shared_ptr<Container>& container, const glm::vec2& point) {
    std::vector<boost::shared_ptr<component::MenuBar>> bars;
    std::vector<boost::shared_ptr<component::Toolbar>> toolbars;
    strips(container, &bars, &toolbars);

    for (const boost::shared_ptr<component::MenuBar>& bar : bars) {
        if (bar && bar->press(point)) {
            return true;
        }
    }
    for (const boost::shared_ptr<component::Toolbar>& bar : toolbars) {
        if (bar && bar->press(point)) {
            return true;
        }
    }

    const boost::shared_ptr<Component> picked = container->pick(point);
    if (!picked) {
        return false;
    }
    held_ = picked;
    act(picked, point);
    return true;
}

bool Cursor::release(const glm::vec2& point) {
    const boost::shared_ptr<Component> holding = held_.lock();
    held_.reset();
    if (!holding) {
        return false;
    }
    if (holding->type() == component::Type::Scrollbar) {
        boost::dynamic_pointer_cast<component::Scrollbar>(holding)->drag(point);
    }
    return true;
}

void Cursor::act(const boost::shared_ptr<Component>& component, const glm::vec2& point) {
    switch (component->type()) {
        case component::Type::SelectList: {
            // which row was clicked is the list's to know and the app's to interpret: the
            // list moves its selection and sends its command, per ADR-0038
            const boost::shared_ptr<component::SelectList> list =
                boost::dynamic_pointer_cast<component::SelectList>(component);
            const int row = list->at(point);
            if (row == component::SelectList::none) {
                return;
            }
            list->selected(row);
            break;
        }
        case component::Type::TabBar: {
            // a tab bar owns which page is up, the way a list owns which row is chosen, so
            // there is nothing to send
            const boost::shared_ptr<component::TabBar> bar =
                boost::dynamic_pointer_cast<component::TabBar>(component);
            const int tab = bar->at(point);
            if (tab != component::TabBar::none) {
                bar->selected(tab);
            }
            return;
        }
        case component::Type::Scrollbar:
            // absolute rather than relative, so a bar picked anywhere on its track jumps to
            // what was clicked and then follows the cursor until the press comes up
            boost::dynamic_pointer_cast<component::Scrollbar>(component)->drag(point);
            return;
        default:
            break;
    }
    dispatch(component);
}

void Cursor::dispatch(const boost::shared_ptr<Component>& component) const {
    if (!dispatcher_) {
        return;
    }
    // a component does not own the state it shows: the click sends the command and marks
    // nothing, and whatever answers it sets checked() - ADR-0019
    switch (component->type()) {
        case component::Type::Button: {
            const boost::shared_ptr<component::Button> button =
                boost::dynamic_pointer_cast<component::Button>(component);
            if (button->event().context()) {
                dispatcher_->trigger(button->event());
            }
            break;
        }
        case component::Type::CheckBox:
        case component::Type::RadioButton: {
            const boost::shared_ptr<component::CheckBox> box =
                boost::dynamic_pointer_cast<component::CheckBox>(component);
            if (box->event().context()) {
                dispatcher_->trigger(box->event());
            }
            break;
        }
        case component::Type::SelectList: {
            const boost::shared_ptr<component::SelectList> list =
                boost::dynamic_pointer_cast<component::SelectList>(component);
            if (list->event().context()) {
                dispatcher_->trigger(list->event());
            }
            break;
        }
        default:
            // a panel, a label, a bar - pickable so that it takes a press off whatever is
            // under it, and carrying no command of its own
            break;
    }
}

};  // namespace v3d::ui
