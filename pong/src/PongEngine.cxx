/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongEngine.h"

#include <api/audio/Engine.h>
#include <api/audio/Loaders.h>
#include <api/ecs/component/Position1D.h>
#include <api/ecs/component/Position2D.h>
#include <api/engine/Engine.h>
#include <api/event/Event.h>
#include <api/event/Source.h>
#include <api/event/State.h>
#include <api/ui/shell/GameMenu.h>

#include <array>
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
 * Each menu item that captures a key, paired with the paddle command it rebinds.
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


PongEngine::PongEngine(const std::string & path) :
    v3d::engine::Engine(path),
    shell_(this, { .menu = true, .statistics = "pong::toggleStatistics" }) {
    // the renderer draws the ball and the paddles between the last two steps
    interpolate<v3d::ecs::component::Position2D>();
    interpolate<v3d::ecs::component::Position1D>();
    // simulate() reads whether each paddle command's key is held
    for (const std::string_view command : PongScene::paddleCommands) {
        commands().addHeld("pong::" + std::string(command));
    }

    // what the menu's items send
    commands().add("ui::setMaxScore", [this](const v3d::event::Event& event) {
        boost::optional<v3d::event::EventData> data = event.data();
        if (data && std::holds_alternative<int>(data.get())) {
            scene_->state().maxScore(std::get<int>(data.get()));
        }
    });
    for (const auto& binding : PADDLE_COMMANDS) {
        commands().add("ui::" + std::string(binding.first), [this](const v3d::event::Event& event) {
            rebindPaddleKey(event);
        });
    }
    commands().addPress("ui::setSingleplayerMode", [this]() { mode(false); });
    commands().addPress("ui::setCoopMode", [this]() { mode(true); });
}

bool PongEngine::start() {
    window()->caption("Pong!");

    soundEngine_ = boost::make_shared<v3d::audio::Engine>(logger(), dispatcher());
    v3d::audio::registerLoaders(*assets(), logger());
    // the return is ignored. When no device opens, the engine logs why and stays silent, and
    // play() returns Voice::None.
    soundEngine_->initialize();

    const v3d::ui::shell::GameMenu::Suspend pause = [this](bool suspended) { scene_->state().pause(suspended); };
    if (!shell_.start(pause)) {
        return false;
    }

    const boost::json::object* sounds = document(v3d::audio::Engine::DOCUMENT);
    if (sounds != nullptr) {
        soundEngine_->load(*sounds, *assets());
    }
    const boost::shared_ptr<v3d::window::Window> win = window();
    renderer_ = boost::make_shared<PongRenderer>(win, logger(), &shell_);
    scene_ = boost::make_shared<PongScene>(&registry(), dispatcher());
    renderer_->scene(scene_);

    // a key captured for a rebinding is taken before it reaches the bindings
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
    // steer by the keys held now, not by the presses and releases received. A key held while
    // the menu was open, or across a change of mode, then moves its paddle as soon as play resumes
    for (const std::string_view command : PongScene::paddleCommands) {
        scene_->steer(command, held("pong::" + std::string(command)));
    }
    scene_->tick(step);
    return true;
}

bool PongEngine::render() {
    renderer_->draw(alpha());
    return true;
}

/**
 **/
bool PongEngine::release() {
    if (soundEngine_) {
        soundEngine_->shutdown();
    }
    // the screen draws through the renderer, so it goes first
    shell_.release();
    if (renderer_) {
        // the device has to be idle before the window it presents to is destroyed
        renderer_->shutdown();
    }
    return true;
}

/**
 **/
void PongEngine::mode(bool coop) {
    scene_->coop(coop);
    scene_->reset();
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
}

/**
 **/
void PongEngine::handleSource(const v3d::event::Source& source) {
    const boost::shared_ptr<v3d::ui::shell::GameMenu>& menu = shell_.menu();
    if (!menu || !menu->capturing() || source.state() != v3d::event::State::Pressed) {
        return;
    }
    // escape is never captured as a paddle key. Its binding steps back out of the menu, which
    // abandons the capture
    if (source.name() == "escape") {
        return;
    }
    // a paddle is steered by keys, and a mouse button shares names with some of them, so
    // only a keyboard source is captured
    if (!source.context() || source.context()->name() != "keyboard") {
        return;
    }
    // the capturing menu item takes the key's name. Consuming the source stops the key's own
    // bindings from firing
    menu->capture(std::string(source.name()));
    source.consume();
}
