/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Event.h>

#include <boost/shared_ptr.hpp>

namespace v3d::ui {

class Component;

/**
 * The command a component sends when it is activated, per ADR-0040.
 *
 * Both routers ask: ui::Cursor for a press and ui::Keys for a return or a space. Which
 * components carry a command is one list rather than one per router, because a component
 * a press activates and a key does not is the defect this exists to make unrepresentable.
 *
 * A component does not own the state it shows - activating one sends its command and
 * marks nothing, and whatever answers the command sets checked(), per ADR-0019. A list
 * and a tab bar are the exception the ADR names: which row or page is chosen is a place
 * in what the component holds rather than a state a command owns, so the router moves it
 * before asking here.
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

};  // namespace v3d::ui
