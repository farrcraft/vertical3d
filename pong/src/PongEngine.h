/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/audio/Engine.h>
#include <api/engine/Engine.h>
#include <api/engine/Settings.h>
#include <api/event/Event.h>
#include <api/event/Source.h>
#include <api/ui/Engine.h>
#include <api/ui/shell/GameMenu.h>

#include <string>

#include "PongScene.h"

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
    bool simulate(float step) override;

    /**
     **/
    bool render() override;

    void handleSource(const v3d::event::Source& source);

 protected:
    /**
     * @return bool
     **/
    bool start() override;

    /**
     * @return bool
     **/
    bool release() override;

 private:
    /**
     * Change the game's mode from the menu. Coop is the only mode that differs: in the others
     * the second paddle is the same opponent.
     **/
    void mode(bool coop);

    /**
     * Point one of the four paddle commands at the key an Options item captured, and store
     * it so the next run starts with it.
     **/
    void rebindPaddleKey(const v3d::event::Event& event);

    /**
     * Apply every paddle key the player has changed, over the ones the config binds.
     **/
    void applyStoredBindings();

    boost::shared_ptr<v3d::audio::Engine> soundEngine_;
    boost::shared_ptr<v3d::engine::Settings> settings_;

    boost::shared_ptr<PongScene> scene_;
    boost::shared_ptr<PongRenderer> renderer_;
    boost::shared_ptr<v3d::ui::Engine> vgui_;
    boost::shared_ptr<v3d::ui::shell::GameMenu> menu_;
    // the dispatcher holds a delegate to this object; this lets it go when the object does
    entt::scoped_connection sources_;
};
