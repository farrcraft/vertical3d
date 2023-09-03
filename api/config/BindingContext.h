/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>
#include <string_view>
#include <unordered_map>

#include <boost/shared_ptr.hpp>

namespace v3d::config {
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
        bool setBinding(const std::string& key, const std::string& binding);

     private:
        std::string context_;
        std::unordered_map<std::string, std::string> bindings_;
    };
};  // namespace v3d::config
