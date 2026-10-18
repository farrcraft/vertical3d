/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Command.h"

#include <api/event/Event.h>
#include <api/ui/Component.h>
#include <api/ui/component/Button.h>
#include <api/ui/component/CheckBox.h>
#include <api/ui/component/SelectList.h>
#include <api/ui/component/Slider.h>
#include <api/ui/component/Type.h>

#include <boost/pointer_cast.hpp>

namespace v3d::ui::input {

bool send(entt::dispatcher* dispatcher, const v3d::event::Event& event) {
    if (dispatcher == nullptr || !event.context()) {
        return false;
    }
    dispatcher->trigger(event);
    return true;
}

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
            // a radio button is a check box with a round mark, and carries its command the same
            // way
            const boost::shared_ptr<component::CheckBox> box =
                boost::dynamic_pointer_cast<component::CheckBox>(component);
            return box ? box->event() : v3d::event::Event();
        }
        case component::Type::SelectList: {
            const boost::shared_ptr<component::SelectList> list =
                boost::dynamic_pointer_cast<component::SelectList>(component);
            return list ? list->event() : v3d::event::Event();
        }
        case component::Type::Slider: {
            const boost::shared_ptr<component::Slider> slider =
                boost::dynamic_pointer_cast<component::Slider>(component);
            return slider ? slider->event() : v3d::event::Event();
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
            // none of these carries a command. A box has only its children, and a strip
            // handles its own press. A bar, a scrollbar and a tab bar change what they show
            // without sending anything. A panel or a label is pickable only so that it takes
            // a press off what is under it. A text box does carry a command, but a click into
            // one starts typing, so only a return sends it, and input::Keys reads the event
            // from the box directly
        case component::Type::TextBox:
            return v3d::event::Event();
    }
    // every enumerator is handled above and the switch carries no default, so C4062 names
    // this function when a component type is added
    return v3d::event::Event();
}

};  // namespace v3d::ui::input
