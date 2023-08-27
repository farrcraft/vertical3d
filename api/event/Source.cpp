/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Source.h"

namespace v3d::event {

    /**
     **/
    Source::Source(const std::string& name, const std::string& scope, const std::string& context) :
        Event(name, scope, context) {
    }

};  // namespace v3d::event
