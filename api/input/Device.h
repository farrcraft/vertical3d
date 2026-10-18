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
     * Handle an event if it belongs to this device.
     *
     * @param event any polled event
     *
     * @return true for every event that belongs to this device, including one it ignores
     **/
    virtual bool handleEvent(const SDL_Event& event) = 0;

    /**
     * Forget the edges this frame recorded. The loop calls this once per frame, after
     * everything that reads the edges has run. A device with no edge state does nothing.
     **/
    virtual void flush() {}

 protected:
    /**
     * @return the event context this device's events are raised in
     **/
    const boost::shared_ptr<v3d::event::Context>& context() const noexcept;

    /**
     * @return the dispatcher this device raises its events through
     **/
    const boost::shared_ptr<entt::dispatcher>& dispatcher() const noexcept;

 private:
    boost::shared_ptr<v3d::event::Context> context_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
};
};  // namespace v3d::input
