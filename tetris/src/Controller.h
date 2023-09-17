/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "../../api/engine/Engine.h"
#include "../../api/log/Logger.h"
#include "../../api/event/Event.h"

#include <entt/entt.hpp>

class TetrisScene;
class TetrisRenderer;

class Controller final : public v3d::engine::Engine {
 public:
    Controller(const std::string& path);

    bool initialize();
    /**
     * @return bool
     **/
    bool tick();

    /**
     **/
    bool render();

    /**
     * @return bool
     **/
    bool shutdown();

    void handleEvent(const v3d::event::Event& event);

 private:
    boost::shared_ptr<TetrisScene> scene_;
    boost::shared_ptr<TetrisRenderer> renderer_;
    entt::registry registry_;
};
