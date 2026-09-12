/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Context.h>

#include <SDL3/SDL.h>

#include <entt/entt.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::input {
/**
 * Base for input devices
 **/
class Device {
 public:
    /**
     **/
    explicit Device(const boost::shared_ptr<v3d::event::Context> & context, const boost::shared_ptr<entt::dispatcher> &dispatcher);

    /**
     **/
    virtual ~Device() = default;

    /**
     * Handle any device events
     * If the event is associated with this device, true should always be returned.
     * 
     * @param event a potential event to handle
     * 
     * @return true if the event was handled by this device
     **/
    virtual bool handleEvent(const SDL_Event& event) = 0;

    /**
     * Forget the edges this frame recorded. Called once per frame by the loop, after
     * everything that reads them has run - a device with no edge state does nothing.
     **/
    virtual void flush() {}

 protected:
    boost::shared_ptr<v3d::event::Context> context_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
};
};  // namespace v3d::input
