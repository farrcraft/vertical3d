/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Cursor.h"

#include <api/ui/Component.h>
#include <api/ui/Container.h>
#include <api/ui/Interface.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/Scrollbar.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/Slider.h>
#include <api/ui/component/TabBar.h>
#include <api/ui/component/TextBox.h>
#include <api/ui/component/Toolbar.h>
#include <api/ui/component/Type.h>
#include <api/ui/component/menu/Bar.h>
#include <api/ui/paint/Text.h>

#include <algorithm>
#include <vector>

#include "Command.h"

namespace v3d::ui::input {

namespace {

/**
 * Collect a container's menu bars and toolbars. The cursor offers a point to the menu bars
 * before the toolbars, the reverse of the draw order, because an open menu drops a panel
 * over a toolbar.
 *
 * A strip that is hidden, disabled or not pickable is left out, as Container::pick() leaves
 * out such a component.
 **/
void strips(const boost::shared_ptr<Container>& container,
    std::vector<boost::shared_ptr<component::menu::Bar>>* bars,
    std::vector<boost::shared_ptr<component::Toolbar>>* toolbars) {
    for (const boost::shared_ptr<Component>& component : container->components()) {
        if (!component || !component->visible() || !component->enabled() || !component->pickable()) {
            continue;
        }
        if (!component::traits(component->type()).strip) {
            continue;
        }
        if (component->type() == component::Type::MenuBar) {
            bars->push_back(boost::dynamic_pointer_cast<component::menu::Bar>(component));
        } else {
            toolbars->push_back(boost::dynamic_pointer_cast<component::Toolbar>(component));
        }
    }
}

/**
 * Set a component's hover state on or off.
 *
 * A button is the only component with a hover state. A Toolbar writes the same state onto
 * its buttons, so a button in the tree is highlighted the same way as one on a toolbar.
 *
 * A button that cannot be used is left alone in both directions, so a transient hover never
 * overwrites its disabled look.
 **/
void lit(const boost::shared_ptr<Component>& component, bool on) {
    if (!component || component->type() != component::Type::Button || !usable(*component)) {
        return;
    }
    const boost::shared_ptr<component::Button> button =
        boost::dynamic_pointer_cast<component::Button>(component);
    if (button) {
        button->state(on ? component::Button::STATE_HOVER : component::Button::STATE_NORMAL);
    }
}

};  // namespace

Cursor::Cursor(const boost::shared_ptr<Interface>& ui, const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const paint::Measure& measure) :
    ui_(ui),
    dispatcher_(dispatcher),
    measure_(measure) {
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
    // a held press follows the cursor wherever it goes, so a thumb dragged off the bar it
    // started on is not lost
    const boost::shared_ptr<Component> holding = held_.lock();
    if (holding) {
        follow(holding, point);
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
        std::vector<boost::shared_ptr<component::menu::Bar>> bars;
        std::vector<boost::shared_ptr<component::Toolbar>> toolbars;
        strips(container, &bars, &toolbars);

        for (const boost::shared_ptr<component::menu::Bar>& bar : bars) {
            taken = (bar && bar->motion(point)) || taken;
        }
        for (const boost::shared_ptr<component::Toolbar>& bar : toolbars) {
            if (!bar) {
                continue;
            }
            // every toolbar is told either way, so that a button the cursor has left, or
            // that an open panel now covers, stops drawing its hover
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
    // a strip that took the point covers whatever is under it, so nothing in the tree is
    // hovered
    hover(over);
    return taken;
}

bool Cursor::press(const glm::vec2& point) {
    if (!ui_) {
        return false;
    }
    // a press moves the focus wherever it lands: onto a focusable component, and otherwise
    // off whatever had it
    ui_->focus(boost::shared_ptr<Component>());
    // any_of stops at the first container that takes the press, so a press never reaches
    // more than one container
    return std::ranges::any_of(ui_->containers(),
        [this, &point](const boost::shared_ptr<Container>& container) {
            return container && container->visible() && press(container, point);
        });
}

bool Cursor::press(const boost::shared_ptr<Container>& container, const glm::vec2& point) {
    std::vector<boost::shared_ptr<component::menu::Bar>> bars;
    std::vector<boost::shared_ptr<component::Toolbar>> toolbars;
    strips(container, &bars, &toolbars);

    for (const boost::shared_ptr<component::menu::Bar>& bar : bars) {
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
    follow(holding, point);
    return true;
}

void Cursor::follow(const boost::shared_ptr<Component>& holding, const glm::vec2& point) const {
    // the components a press drags. Exhaustive, so a type added to the enum fails the build
    // here until it says whether it follows the cursor
    switch (holding->type()) {
        case component::Type::Scrollbar:
            boost::dynamic_pointer_cast<component::Scrollbar>(holding)->drag(point);
            break;
        case component::Type::Slider:
            if (boost::dynamic_pointer_cast<component::Slider>(holding)->drag(point)) {
                dispatch(holding);
            }
            break;
        case component::Type::TextBox:
            // the press left the anchor where it landed, so following the cursor selects the
            // run between the two
            place(boost::dynamic_pointer_cast<component::TextBox>(holding), point, true);
            break;
        case component::Type::Undefined:
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
        case component::Type::SelectList:
        case component::Type::TabBar:
        case component::Type::TabPage:
        case component::Type::Toolbar:
        case component::Type::VerticalBox:
            break;
    }
}

void Cursor::act(const boost::shared_ptr<Component>& component, const glm::vec2& point) {
    switch (component->type()) {
        case component::Type::SelectList: {
            // the list moves its selection and sends its command; what the row means is
            // for the app to decide
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
            // a tab bar carries no command. A press on a tab only changes the chosen page
            const boost::shared_ptr<component::TabBar> bar =
                boost::dynamic_pointer_cast<component::TabBar>(component);
            const int tab = bar->at(point);
            if (tab != component::TabBar::none) {
                bar->selected(tab);
            }
            return;
        }
        case component::Type::Scrollbar:
            // absolute, not relative: a press anywhere on the track jumps the bar there, and
            // the bar then follows the cursor until the press comes up
            boost::dynamic_pointer_cast<component::Scrollbar>(component)->drag(point);
            return;
        case component::Type::Slider:
            // like a scrollbar, a press anywhere on the track jumps the thumb there, and a
            // slider sends its command only when that changed its value
            if (boost::dynamic_pointer_cast<component::Slider>(component)->drag(point)) {
                dispatch(component);
            }
            return;
        case component::Type::TextBox:
            // the caret and the selection anchor both move to where the press landed in the
            // text, so a drag then selects from there
            place(boost::dynamic_pointer_cast<component::TextBox>(component), point, false);
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
        case component::Type::Toolbar:
        case component::Type::Undefined:
        case component::Type::VerticalBox:
            // a press does not move anything inside these, so it only sends the command,
            // through dispatch() after the switch
            break;
    }
    dispatch(component);
}

void Cursor::place(const boost::shared_ptr<component::TextBox>& box, const glm::vec2& point,
    bool extend) const {
    if (!box || !measure_) {
        // a cursor with no Measure cannot find a character, so the caret stays where it was
        return;
    }
    box->caret(box->at(point, measure_), extend);
}

void Cursor::dispatch(const boost::shared_ptr<Component>& component) const {
    // a component does not own the state it shows. The click sends the command and marks
    // nothing, and whatever handles the command sets checked(). input::command() decides
    // which components carry a command, for a key and a click alike
    send(dispatcher_.get(), command(component));
}

};  // namespace v3d::ui::input
