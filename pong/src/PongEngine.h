/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>

#include "PongScene.h"

#include "../../api/engine/Engine.h"
#include "../../api/audio/Engine.h"
#include "../../api/event/Event.h"
#include "../../api/ui/Engine.h"

#include <entt/entt.hpp>

class PongRenderer;

/**
 * The core game engine class
 */
class PongEngine final : public v3d::engine::Engine {
 public:
    explicit PongEngine(const std::string & path);

    /**
     * @return bool
     **/
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

    // protected:
    // void setMenuItemDefaults(const boost::shared_ptr<Luxa::Menu> & menu);

 private:
    boost::shared_ptr<v3d::audio::Engine> soundEngine_;

    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<PongRenderer> renderer_;
    boost::shared_ptr<v3d::ui::Engine> vgui_;

    entt::registry registry_;
};
