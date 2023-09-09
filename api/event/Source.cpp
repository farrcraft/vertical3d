/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "Source.h"

namespace v3d::event {

    /**
     **/
    Source::Source(const std::string& name, const boost::shared_ptr<Context>& context) :
        Event(name, context) {
    }

};  // namespace v3d::event
