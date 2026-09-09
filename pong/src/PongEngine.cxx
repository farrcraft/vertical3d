/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongEngine.h"

#include <api/asset/Sound.h>
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

bool::PongEngine::initialize() {
    if (!Engine::initialize(
        static_cast<int>(v3d::engine::Feature::Window |
        v3d::engine::Feature::KeyboardInput |
        v3d::engine::Feature::MouseInput |
        v3d::engine::Feature::Config))) {
        return false;
    }

    window_->caption("Pong!");

    // after Engine::initialize(), because rebinding rebuilds the mapper the config built
    settings_ = boost::make_shared<v3d::engine::Settings>(ORGANIZATION, APPLICATION, logger_);
    settings_->load();
    applyStoredBindings();

    soundEngine_ = boost::make_shared<v3d::audio::Engine>(logger_, dispatcher_);
    // the return is not read: a device that will not open leaves the engine silent, and the
    // engine logs why. Every clip played against it is a false return.
    soundEngine_->initialize();

    vgui_ = boost::make_shared<v3d::ui::Engine>(eventEngine_, dispatcher_, logger_);
    menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(vgui_, [this](bool suspended) {
        scene_->state().pause(suspended);
    });

    if (config_) {
        boost::shared_ptr<v3d::asset::Json> soundConfig = config_->get(v3d::config::Type::Sound);
        if (soundConfig) {
            // a clip is an asset like any other, so the file the config names is resolved
            // against the manager's path rather than the working directory
            soundEngine_->load(soundConfig,
                [this](const std::string& source) -> boost::shared_ptr<v3d::audio::AudioClip> {
                    boost::shared_ptr<v3d::asset::Sound> asset = boost::dynamic_pointer_cast<v3d::asset::Sound>(
                        assetManager_->load(source, v3d::asset::Type::AudioWav));
                    if (!asset) {
                        return boost::shared_ptr<v3d::audio::AudioClip>();
                    }
                    return asset->clip();
                });
        }

        boost::shared_ptr<v3d::asset::Json> uiConfig = config_->get(v3d::config::Type::Ui);
        if (uiConfig) {
            if (!vgui_->load(uiConfig)) {
                return false;
            }
        }
    }
    boost::shared_ptr<v3d::render::realtime::Window> win = window();
    renderer_ = boost::make_shared<PongRenderer>(win, logger_, assetManager_, &registry_);
    scene_ = boost::make_shared<PongScene>(&registry_, dispatcher_);
    renderer_->scene(scene_);
    renderer_->ui(vgui_);

    // register game commands
    dispatcher_->sink<v3d::event::Event>().connect<&PongEngine::handleEvent>(*this);

    // set the scene size according to the window canvas
    renderer_->resize(window_->width(), window_->height());

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
    scene_->tick(step);
    return true;
}

bool PongEngine::render() {
    const v3d::engine::Statistics& measured = statistics();
    renderer_->draw({ measured.mean(), measured.last(), measured.steps() });
    return true;
}

/**
 **/
bool PongEngine::shutdown() {
    if (soundEngine_) {
        soundEngine_->shutdown();
    }
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    if (!v3d::engine::Engine::shutdown()) {
        return false;
    }
    return true;
}

void PongEngine::handlePlayEvent(const v3d::event::Event& event) {
    // play commands
    // the paddle moves while its key is held, so these follow the event's edge
    bool held = (event.state() == v3d::event::State::Pressed);
    if (event.name() == "leftPaddleUp") {
        if (!scene_->state().paused()) {
            scene_->left().up(held);
        }
    } else if (event.name() == "leftPaddleDown") {
        if (!scene_->state().paused()) {
            scene_->left().down(held);
        }
    } else if (event.name() == "rightPaddleUp") {
        if (!scene_->state().paused() && scene_->state().coop()) {
            scene_->right().up(held);
        }
    } else if (event.name() == "rightPaddleDown") {
        if (!scene_->state().paused() && scene_->state().coop()) {
            scene_->right().down(held);
        }
    } else if (event.name() == "showGameMenu") {
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
        scene_->state().coop(false);
        scene_->reset();
    } else if (event.name() == "setCoopMode") {
        scene_->state().coop(true);
        scene_->reset();
    } else if (event.name() == "quit") {
        // not shutdown() - this is running inside the event loop, which would tick and
        // render one more frame against the window shutdown() had destroyed
        quit();
        return;
    }

    if (event.name() == "showGameMenu") {
        menu_->toggle();
        return;
    }

    menu_->navigate(event.name());
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
    logger_->get()->info("bound {} to {}", command, key);

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
            logger_->get()->info("bound {} to {} from settings", binding.second, key);
        }
    }
}

/**
 **/
void PongEngine::handleEvent(const v3d::event::Event& event) {
    // a menu item capturing a key wants the key rather than what it is bound to, so a
    // source event goes to the capture and no further while one is open
    if (event.type() == v3d::event::Type::Source && menu_ && menu_->capturing()) {
        if (event.state() == v3d::event::State::Pressed) {
            menu_->capture(std::string(event.name()));
        }
        return;
    }

    if (event.context()->name() == "pong") {
        handlePlayEvent(event);
        return;
    }
    if (event.context()->name() == "ui") {
        handleUiEvent(event);
    }
}
