/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Source.h"

#include <string>

namespace v3d::event {

Source::Source(const std::string& name, const boost::shared_ptr<Context>& context, State state) :
    Event(name, context),
    consumed_(boost::make_shared<bool>(false)) {
    type(Type::Source);
    this->state(state);
}

void Source::consume() const noexcept {
    *consumed_ = true;
}

bool Source::consumed() const noexcept {
    return *consumed_;
}

void publish(entt::dispatcher& dispatcher, const Source& source) {
    dispatcher.trigger(source);
    if (!source.consumed()) {
        dispatcher.trigger(Unclaimed{&source});
    }
}

};  // namespace v3d::event
