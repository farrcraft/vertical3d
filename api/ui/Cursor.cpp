/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Cursor.h"

#include <algorithm>
#include <vector>

#include "Command.h"
#include "Component.h"
#include "Container.h"
#include "Engine.h"
#include "component/Bar.h"
#include "component/Button.h"
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

/**
 * Light a component up, or put it back to normal.
 *
 * A button is the only component that has a state to write, and it is the state a
 * Toolbar writes onto the buttons it holds, so a button in a tree lights up the way one
 * on a strip does rather than by a second mechanism.
 **/
void lit(const boost::shared_ptr<Component>& component, bool on) {
    if (!component || component->type() != component::Type::Button) {
        return;
    }
    const boost::shared_ptr<component::Button> button =
        boost::dynamic_pointer_cast<component::Button>(component);
    if (button) {
        button->state(on ? component::Button::STATE_HOVER : component::Button::STATE_NORMAL);
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

boost::shared_ptr<Component> Cursor::hovered() const {
    return hovered_.lock();
}

void Cursor::hover(const boost::shared_ptr<Component>& component) {
    const boost::shared_ptr<Component> was = hovered_.lock();
    if (was == component) {
        return;
    }
    lit(was, false);
    lit(component, true);
    hovered_ = component;
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
    boost::shared_ptr<Component> over;
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
        if (!taken) {
            over = container->pick(point);
            taken = static_cast<bool>(over);
        }
    }
    // a strip that took the point covers whatever is under it, so the tree is left with
    // nothing hovered rather than with what the cursor would have been over
    hover(over);
    return taken;
}

bool Cursor::press(const glm::vec2& point) {
    if (!ui_) {
        return false;
    }
    // a press is what says "type here", so it moves the focus wherever it lands - onto a
    // component that asked to be focusable, and off whatever had it otherwise. ADR-0040
    ui_->focus(boost::shared_ptr<Component>());
    // any_of stops at the first container that takes the press, which is what keeps a
    // press from reaching more than one ui
    return std::ranges::any_of(ui_->containers(),
        [this, &point](const boost::shared_ptr<Container>& container) {
            return container && container->visible() && press(container, point);
        });
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
    ui_->focus(picked);
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
        case component::Type::Bar:
        case component::Type::Button:
        case component::Type::CheckBox:
        case component::Type::HorizontalBox:
        case component::Type::Icon:
        case component::Type::Label:
        case component::Type::Menu:
        case component::Type::MenuBar:
        case component::Type::MenuItem:
        case component::Type::Panel:
        case component::Type::RadioButton:
        case component::Type::TabPage:
        case component::Type::TextBox:
        case component::Type::Toolbar:
        case component::Type::Undefined:
        case component::Type::VerticalBox:
            // nothing here owns a place a press moves it to, so the press is the command and
            // nothing else - which is what falls out of the switch into dispatch()
            break;
    }
    dispatch(component);
}

void Cursor::dispatch(const boost::shared_ptr<Component>& component) const {
    if (!dispatcher_) {
        return;
    }
    // a component does not own the state it shows: the click sends the command and marks
    // nothing, and whatever answers it sets checked() - ADR-0019. Which components carry
    // one is ui::command()'s to know, shared with the key that activates the same thing
    const v3d::event::Event sent = command(component);
    if (sent.context()) {
        dispatcher_->trigger(sent);
    }
}

};  // namespace v3d::ui
