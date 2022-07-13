/**
 * The Untitled Adventure / Odyssey
 * Copyright (c) 2022 Joshua Farr (josh@farrcraft.com)
 **/

#pragma once

#include "../asset/Json.h"

#include <boost/shared_ptr.hpp>

#include <map>
#include <string_view>

namespace odyssey::config {
	/**
	 * A key binding maps an input device key sequence to a system action
	 * this might also depend on context
	 *
	 * e.g.
	 *	keyboard::w::down
	 *		gameplay context - move character up or forward
	 *		ui context - select previous ui item
	 **/
	class BindingContext final {
	public:
		/**
		 **/
		BindingContext(const std::string_view &context);

		/**
		 * Map an input to an action
		 * 
		 * @param key The stringname representation of the input, e.g. "keyboard::w::down"
		 * @param binding The stringname representation of the action, e.g. "player::movement::forward"
		 * 
		 * @return bool true if setting this binding overrides a previous binding
		 **/
		bool setBinding(const std::string_view& key, const std::string_view& binding);

	private:
		std::string_view context_;
		std::unordered_map<std::string_view, std::string_view> bindings_;
	};
};