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
 * Both routers call this: ui::Cursor for a press and ui::Keys for a return or a space.
 * Which components carry a command is one list shared by both routers, so a press and a
 * key always activate the same components.
 *
 * A component does not own the state it shows: activating one sends its command and marks
 * nothing, and whatever handles the command sets checked(). A list and a tab bar are the
 * exception. Which row or page is chosen is a position in what the component holds rather
 * than a state a command owns, so the router moves it before calling this.
 *
 * A text box carries a command and is not activated by either router: a click into one is
 * somebody starting to type and a space is a space, so only a return sends it and ui::Keys
 * reaches for the event itself. Asking here would submit a box the moment it was clicked.
 *
 * @return the event to send, or one with no context when the component carries none.
 *         An event with no context is not dispatchable, so a caller tests context()
 *         rather than being handed something it has to know not to send
 **/
v3d::event::Event command(const boost::shared_ptr<Component>& component);

/**
 * Send a command, if it is one.
 *
 * Every place the ui sends a command comes through here: the cursor, the keys, a strip's
 * button and a menu's item. The one rule about sending is therefore written once. An event with no
 * context is not bound to anything, and Event::str() dereferences the context, so such an
 * event is never sent.
 *
 * @return whether it was sent
 **/
bool send(entt::dispatcher* dispatcher, const v3d::event::Event& event);

};  // namespace v3d::ui::input
