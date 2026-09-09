/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Command.h"

#include <boost/pointer_cast.hpp>

#include "Component.h"
#include "component/Button.h"
#include "component/CheckBox.h"
#include "component/SelectList.h"
#include "component/Type.h"

namespace v3d::ui {

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
        default:
            // a panel, a label, a bar, a tab bar - pickable and focusable so that they take
            // a press or a key off whatever is under them, and carrying no command of their own.
            // A text box is here too: it carries a command, but a click into one is somebody
            // starting to type rather than saying they are done, so only a return sends it
            return v3d::event::Event();
    }
}

};  // namespace v3d::ui
