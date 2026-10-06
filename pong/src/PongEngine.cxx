/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongEngine.h"

#include <api/audio/Loaders.h>
#include <api/ecs/Previous.h>
#include <api/ecs/component/Color3.h>
#include <api/ecs/component/Position1D.h>
#include <api/ecs/component/Position2D.h>
#include <api/engine/Feature.h>

#include <array>
#include <iostream>
#include <map>
#include <utility>
#include <string>
#include <string_view>
#include <variant>

#include "PongRenderer.h"
#include "PongScene.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * The organization and app userPath() resolves against. Never changed: every player's
 * settings live under this pair and a new one orphans them.
 **/
const char* const ORGANIZATION = "Vertical3D";
const char* const APPLICATION = "Pong";

/**
 * The menu item that captures a key, against the paddle command it drives. The item name is
 * also the settings key, so what is stored says which menu wrote it.
 **/
// an array of views rather than a map of strings, because a map with static storage
// duration allocates during static initialization and can throw where nothing can catch it
constexpr std::array<std::pair<std::string_view, std::string_view>, 4> PADDLE_COMMANDS = {{
    {"setLeftPaddleUpKey", "pong::leftPaddleUp"},
    {"setLeftPaddleDownKey", "pong::leftPaddleDown"},
    {"setRightPaddleUpKey", "pong::rightPaddleUp"},
    {"setRightPaddleDownKey", "pong::rightPaddleDown"}
}};

/**
 * @return the command the named menu item drives, or an empty view for an item that drives
 *         no paddle
 **/
std::string_view paddleCommand(std::string_view item) {
    for (const auto& binding : PADDLE_COMMANDS) {
        if (binding.first == item) {
            return binding.second;
        }
    }
    return std::string_view();
}

};  // namespace


PongEngine::PongEngine(const std::string & path) : v3d::engine::Engine(path) {
}

bool PongEngine::start() {
    window()->caption("Pong!");

    // once the engine is up, because rebinding rebuilds the mapper the config built
    settings_ = boost::make_shared<v3d::engine::Settings>(ORGANIZATION, APPLICATION, logger());
    settings_->load();
    applyStoredBindings();

    soundEngine_ = boost::make_shared<v3d::audio::Engine>(logger(), dispatcher());
    v3d::audio::registerLoaders(*assets(), logger());
    // the return is not read: a device that will not open leaves the engine silent, and the
    // engine logs why. Every clip played against it is a false return.
    soundEngine_->initialize();

    vgui_ = boost::make_shared<v3d::ui::Engine>(events(), dispatcher(), logger());
    menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(vgui_, [this](bool suspended) {
        scene_->state().pause(suspended);
    });

    const boost::json::object* sounds = document(v3d::config::Type::Sound);
    if (sounds) {
        soundEngine_->load(*sounds, *assets());
    }
    const boost::json::object* ui = document(v3d::config::Type::Ui);
    if (ui && !vgui_->load(*ui)) {
        return false;
    }
    boost::shared_ptr<v3d::render::realtime::Window> win = window();
    renderer_ = boost::make_shared<PongRenderer>(win, logger(), assets());
    scene_ = boost::make_shared<PongScene>(&registry_, dispatcher());
    renderer_->scene(scene_);
    renderer_->ui(vgui_);

    // register game commands
    events_ = dispatcher()->sink<v3d::event::Event>().connect<&PongEngine::handleEvent>(*this);
    sources_ = dispatcher()->sink<v3d::event::Source>().connect<&PongEngine::handleSource>(*this);

    // reset scene & game state
    scene_->reset();

    return true;
}

/**
 **/
bool PongEngine::simulate(float step) {
    if (!v3d::engine::Engine::simulate(step)) {
        return false;
    }
    // where the ball and paddles were before this step moves them, so the renderer can
    // interpolate between the two steps
    v3d::ecs::snapshot<v3d::ecs::component::Position2D>(registry_);
    v3d::ecs::snapshot<v3d::ecs::component::Position1D>(registry_);
    // the paddles follow the keys held now rather than the presses and releases that arrived,
    // so a key held through the menu or a change of mode moves its paddle as soon as it can
    for (const std::string_view command : PongScene::paddleCommands) {
        scene_->steer(command, held("pong::" + std::string(command)));
    }
    scene_->tick(step);
    return true;
}

bool PongEngine::render() {
    const v3d::engine::Statistics& measured = statistics();
    renderer_->draw({ measured.mean(), measured.last(), measured.steps() }, alpha());
    return true;
}

/**
 **/
bool PongEngine::release() {
    if (soundEngine_) {
        soundEngine_->shutdown();
    }
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    return true;
}

void PongEngine::handlePlayEvent(const v3d::event::Event& event) {
    // a paddle command is read held in simulate() rather than taken as an event
    for (const std::string_view command : PongScene::paddleCommands) {
        if (event.name() == command) {
            return;
        }
    }
    if (event.repeat()) {
        // the rest toggle, and a held key would flick them on and off at the repeat rate
        return;
    }
    if (event.name() == "showGameMenu") {
        menu_->toggle();
    } else if (event.name() == "toggleStatistics") {
        renderer_->statistics()->toggle();
    }
}

void PongEngine::handleUiEvent(const v3d::event::Event& event) {
    if (event.name() == "setMaxScore") {
        boost::optional<v3d::event::EventData> data = event.data();
        if (data) {
            unsigned int maxScore = std::get<int>(data.get());
            scene_->state().maxScore(maxScore);
        }
    } else if (event.name() == "setLeftPaddleUpKey" || event.name() == "setLeftPaddleDownKey" ||
               event.name() == "setRightPaddleUpKey" || event.name() == "setRightPaddleDownKey") {
        rebindPaddleKey(event);
    } else if (event.name() == "setSingleplayerMode" || event.name() == "setMultiplayerMode") {
        // coop is the only mode that differs; the second paddle is the same opponent
        scene_->coop(false);
        scene_->reset();
    } else if (event.name() == "setCoopMode") {
        scene_->coop(true);
        scene_->reset();
    }
}

/**
 **/
void PongEngine::rebindPaddleKey(const v3d::event::Event& event) {
    boost::optional<v3d::event::EventData> data = event.data();
    if (!data || !std::holds_alternative<std::string>(data.get())) {
        return;
    }
    const std::string key = std::get<std::string>(data.get());

    const std::string_view command = paddleCommand(event.name());
    if (command.empty()) {
        return;
    }
    if (!rebind(std::string(command), key)) {
        return;
    }
    logger()->get()->info("bound {} to {}", command, key);

    // stored as it is made rather than on the way out: there is no exit path that reliably
    // runs, and a crash after a rebinding should not lose the rebinding
    settings_->set(std::string(event.name()), key);
    settings_->save();
}

/**
 **/
void PongEngine::applyStoredBindings() {
    for (const auto& binding : PADDLE_COMMANDS) {
        const std::string key = settings_->text(std::string(binding.first), std::string());
        if (key.empty()) {  // untouched, so it keeps tracking whatever the config binds
            continue;
        }
        if (rebind(std::string(binding.second), key)) {
            logger()->get()->info("bound {} to {} from settings", binding.second, key);
        }
    }
}

/**
 **/
void PongEngine::handleSource(const v3d::event::Source& source) {
    if (!menu_ || !menu_->capturing() || source.state() != v3d::event::State::Pressed) {
        return;
    }
    // escape is left to what it is bound to, which steps back out of the menu and abandons
    // the capture - so it is never captured as a paddle key
    if (source.name() == "escape") {
        return;
    }
    // a menu item capturing a key takes the key itself rather than the command it is bound
    // to, so the key is consumed and its bindings do not fire
    menu_->capture(std::string(source.name()));
    source.consume();
}

void PongEngine::handleEvent(const v3d::event::Event& event) {
    if (event.context()->name() == "pong") {
        handlePlayEvent(event);
        return;
    }
    if (event.context()->name() == "ui") {
        handleUiEvent(event);
    }
}
