/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <string>


#include "../../api/engine/Engine.h"
#include "../../api/audio/Engine.h"
#include "../../luxa/luxa/ComponentManager.h"
#include "../../luxa/luxa/menu/Menu.h"

#include <entt/entt.hpp>

class PongScene;
class PongRenderer;

/**
 * The core game controller class
 */
class PongController final : public v3d::engine::Engine {
    
 public:
    explicit PongController(const std::string_view & path);

    /**
     * @return bool
     **/
    bool initialize();

    /**
     * @return bool
     **/
    bool shutdown();

    bool exec(const v3d::command::CommandInfo & command, const std::string & param);
    bool execUI(const v3d::command::CommandInfo & command, const std::string & param);

 protected:
    void setMenuItemDefaults(const boost::shared_ptr<Luxa::Menu> & menu);

 private:
    boost::shared_ptr<v3d::audio::Engine> soundEngine_;

    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<PongRenderer> renderer_;

    boost::shared_ptr<Luxa::ComponentManager> vgui_;

    entt::registry registry_;
};
