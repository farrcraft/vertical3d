/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Engine.h>

#include <SDL3/SDL.h>

#include <vector>

#include "Device.h"
#include "DeviceType.h"
#include "Keyboard.h"
#include "KeyState.h"
#include "Mouse.h"
#include "MouseState.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::input {
/**
 * Owns the input devices an app asked for, the keyboard and the mouse, and offers each polled
 * event to them.
 **/
class Engine final {
 public:
    /**
     **/
    Engine(const boost::shared_ptr<v3d::event::Engine> & eventEngine, const boost::shared_ptr<entt::dispatcher> &dispatcher, DeviceTypes devices);

    /**
     **/
    bool filterEvent(const SDL_Event& event);

    /**
     * Forget the edges every device recorded this frame. Called once per frame by the loop,
     * so an app does not have to arrange for it to happen exactly once.
     **/
    void flush();

    /**
     * @return the keys held and this frame's edges, or nullptr when the engine was built
     *         without DeviceType::Keyboard
     **/
    const KeyState* keys() const;

    /**
     * @return the buttons held, the cursor position and this frame's edges, or nullptr when
     *         the engine was built without DeviceType::Mouse
     **/
    const MouseState* mouse() const;

 private:
    std::vector<boost::shared_ptr<Device> > devices_;
    /**< the same two devices as above where they were requested, kept separately so the
         state they own can be reached without checking what kind each element is **/
    boost::shared_ptr<Keyboard> keyboard_;
    boost::shared_ptr<Mouse> mouse_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    boost::shared_ptr<v3d::event::Engine> eventEngine_;
};

};  // namespace v3d::input
