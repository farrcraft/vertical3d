/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::ui {

class Component;

};  // namespace v3d::ui

namespace v3d::ui::input {

/**
 * The command a component sends when it is activated.
 *
 * input::Cursor calls this for a press, and input::Keys for a return or a space. Both use
 * this one list of which components carry a command, so a press and a key activate the
 * same components.
 *
 * A component does not own the state it shows. Activating one sends its command and marks
 * nothing, and whatever handles the command sets checked(). A list and a tab bar are the
 * exception. The chosen row or page is a position in what the component holds, so Cursor
 * and Keys move it before calling this.
 *
 * A text box carries a command, but this returns no event for one. A click into a text box
 * starts typing, and a space is typed as a space. Only a return submits the box, and
 * input::Keys reads that event from the box directly.
 *
 * @return the event to send, or one with no context when the component carries none.
 *         An event with no context cannot be dispatched, so a caller tests context()
 *         before sending
 **/
v3d::event::Event command(const boost::shared_ptr<Component>& component);

/**
 * Send a command, if it is one.
 *
 * Every command the ui sends goes through here, from the cursor, the keys, a toolbar
 * button or a menu item. An event with no context is not bound to anything, so it is never
 * sent. Nor is any event sent when the dispatcher is null.
 *
 * @return whether it was sent
 **/
bool send(entt::dispatcher* dispatcher, const v3d::event::Event& event);

};  // namespace v3d::ui::input
