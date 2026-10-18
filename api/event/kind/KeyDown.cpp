/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "KeyDown.h"

#include <api/event/Context.h>

#include "Key.h"

#include <string>

namespace v3d::event::kind {

/**
 **/
KeyDown::KeyDown(const std::string& name, const boost::shared_ptr<Context>& context) :
    Key(name, context, true) {
}

};  // namespace v3d::event::kind
