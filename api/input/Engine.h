/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Engine.h>

#include <SDL3/SDL.h>

#include <vector>

#include "Device.h"
#include "Keyboard.h"
#include "KeyState.h"
#include "Mouse.h"
#include "MouseState.h"

#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::input {
/**
 * The Input Engine is responsible for handling input from supported/registered
 * devices, e.g. mouse, keyboard.
 **/
class Engine final {
 public:
    /**
     **/
    Engine(const boost::shared_ptr<v3d::event::Engine> & eventEngine, const boost::shared_ptr<entt::dispatcher> &dispatcher, int devices);

    /**
     **/
    bool filterEvent(const SDL_Event& event);

    /**
     * Forget the edges every device recorded this frame. Called once per frame by the loop,
     * which is what makes "exactly once" something an app does not have to arrange.
     **/
    void flush();

    /**
     * @return what the keyboard holds and what changed edge this frame, or nullptr when the
     *         engine was built without DeviceType::Keyboard
     **/
    const KeyState* keys() const;

    /**
     * @return what the mouse holds, where its cursor is and what changed edge this frame, or
     *         nullptr when the engine was built without DeviceType::Mouse
     **/
    const MouseState* mouse() const;

 private:
    std::vector<boost::shared_ptr<Device> > devices_;
    /**< the same two devices as above where they were asked for, kept apart so the state
         they own can be reached without asking what kind each element is **/
    boost::shared_ptr<Keyboard> keyboard_;
    boost::shared_ptr<Mouse> mouse_;
    boost::shared_ptr<entt::dispatcher> dispatcher_;
    boost::shared_ptr<v3d::event::Engine> eventEngine_;
};

};  // namespace v3d::input
