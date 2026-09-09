/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Command.h"

#include <api/ui/Component.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/CheckBox.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/Type.h>

#include <boost/pointer_cast.hpp>

namespace v3d::ui::input {

v3d::event::Event command(const boost::shared_ptr<Component>& component) {
    if (!component) {
        return v3d::event::Event();
    }
    switch (component->type()) {
        case component::Type::Button: {
            const boost::shared_ptr<component::Button> button =
                boost::dynamic_pointer_cast<component::Button>(component);
            return button ? button->event() : v3d::event::Event();
        }
        case component::Type::CheckBox:
        case component::Type::RadioButton: {
            // a radio button is a check box with a round mark, and the same command under it
            const boost::shared_ptr<component::CheckBox> box =
                boost::dynamic_pointer_cast<component::CheckBox>(component);
            return box ? box->event() : v3d::event::Event();
        }
        case component::Type::SelectList: {
            const boost::shared_ptr<component::SelectList> list =
                boost::dynamic_pointer_cast<component::SelectList>(component);
            return list ? list->event() : v3d::event::Event();
        }
        case component::Type::Bar:
        case component::Type::HorizontalBox:
        case component::Type::Icon:
        case component::Type::Label:
        case component::Type::Menu:
        case component::Type::MenuBar:
        case component::Type::MenuItem:
        case component::Type::Panel:
        case component::Type::Scrollbar:
        case component::Type::TabBar:
        case component::Type::TabPage:
        case component::Type::Toolbar:
        case component::Type::Undefined:
        case component::Type::VerticalBox:
            // nothing here carries a command. A box is whatever it holds, a strip routes its
            // own press, a bar and a tab bar own what they show rather than sending it, and a
            // panel or a label is pickable only so that it takes a press off what is under it.
            // A text box is here for a different reason: it carries a command, but a click into
            // one is somebody starting to type rather than saying they are done, so only a
            // return sends it and ui::Keys reaches for the event itself
        case component::Type::TextBox:
            return v3d::event::Event();
    }
    // every enumerator is handled above and the switch carries no default, so C4062 names
    // this function when a component type is added - see ADR-0047
    return v3d::event::Event();
}

};  // namespace v3d::ui::input
