/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shell.h"

#include <api/asset/kind/Json.h>
#include <api/asset/Manager.h>
#include <api/asset/Manifest.h>
#include <api/engine/Engine.h>
#include <api/engine/Statistics.h>
#include <api/event/Bindings.h>
#include <api/event/Chord.h>
#include <api/event/Context.h>
#include <api/event/Event.h>
#include <api/event/kind/DisplayScale.h>
#include <api/window/Window.h>
#include <api/ui/Interface.h>
#include <api/ui/shell/GameMenu.h>
#include <api/ui/shell/Screen.h>
#include <api/ui/shell/StatisticsOverlay.h>

#include <string>

#include <boost/make_shared.hpp>

namespace v3d::shell {

const char* const Shell::gameTheme = "ui/game.json";

/**
 **/
v3d::ui::shell::Screen::Options Shell::game() {
    v3d::ui::shell::Screen::Options options;
    options.size = bodySize;
    options.scaling = v3d::ui::shell::Screen::Scaling::Height;
    return options;
}

/**
 **/
Shell::Shell(v3d::engine::Engine* engine, const Options& options) :
    engine_(engine),
    options_(options) {
    if (options_.menu) {
        v3d::ui::shell::GameMenu::declare(engine_->commands(), menu_);
    }
    if (!options_.statistics.empty()) {
        engine_->commands().addPress(options_.statistics, [this]() {
            if (screen_ && screen_->statistics()) {
                screen_->statistics()->toggle();
            }
        });
    }
}

/**
 **/
bool Shell::start(const v3d::ui::shell::GameMenu::Suspend& suspend) {
    ui_ = boost::make_shared<v3d::ui::Interface>(engine_->events(), engine_->dispatcher(), engine_->logger());
    if (options_.menu) {
        menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(ui_, suspend);
    }
    const boost::shared_ptr<v3d::asset::Manifest>& manifest = engine_->manifest();
    const boost::shared_ptr<v3d::asset::kind::Json> document =
        manifest ? manifest->get(v3d::ui::Interface::DOCUMENT) : boost::shared_ptr<v3d::asset::kind::Json>();
    if (document && !ui_->load(document->document())) {
        return false;
    }
    if (options_.theme.empty()) {
        return true;
    }
    const boost::shared_ptr<v3d::asset::kind::Json> beneath = engine_->assets()->load<v3d::asset::kind::Json>(options_.theme);
    if (!beneath) {
        engine_->logger()->get()->error("The theme beneath the ui, {}, did not load", options_.theme);
        return false;
    }
    return ui_->beneath(beneath->document());
}

/**
 **/
void Shell::present(v3d::render::realtime::WindowRenderer* renderer, const v3d::ui::shell::Screen::Options& options) {
    v3d::ui::shell::Screen::Options shown = options;
    if (!shown.shortcut) {
        // read from the bindings as they stand when the menu is drawn, so a rebinding shows
        // at once. A menu has room for one shortcut, so a command bound to several keys shows
        // the first the bindings list
        shown.shortcut = [engine = engine_](const v3d::event::Event& command) {
            const boost::shared_ptr<v3d::event::Bindings> bindings = engine->bindings();
            if (!bindings) {
                return std::string();
            }
            for (const v3d::event::Event& source : bindings->sources(command.str())) {
                if (source.context() && source.context()->name() == "keyboard") {
                    return v3d::event::printed(source.name());
                }
            }
            return std::string();
        };
    }
    screen_ = boost::make_shared<v3d::ui::shell::Screen>(renderer, engine_->assets(), engine_->logger(), shown);
    // the monitor's scale now, and whenever it changes after
    if (engine_->window()) {
        screen_->display(engine_->window()->displayScale());
    }
    scaled_ = engine_->dispatcher()->sink<v3d::event::kind::DisplayScale>().connect<&Shell::rescale>(*this);
}

/**
 **/
void Shell::rescale(const v3d::event::kind::DisplayScale& event) {
    if (screen_) {
        screen_->display(event.scale());
    }
}

/**
 **/
void Shell::release() {
    scaled_.release();
    screen_.reset();
    menu_.reset();
    ui_.reset();
}

/**
 **/
const boost::shared_ptr<v3d::ui::Interface>& Shell::ui() const noexcept {
    return ui_;
}

/**
 **/
const boost::shared_ptr<v3d::ui::shell::GameMenu>& Shell::menu() const noexcept {
    return menu_;
}

/**
 **/
v3d::ui::shell::Screen* Shell::screen() noexcept {
    return screen_.get();
}

/**
 **/
v3d::ui::shell::StatisticsOverlay::Sample Shell::sample() const {
    const v3d::engine::Statistics& measured = engine_->statistics();
    v3d::ui::shell::StatisticsOverlay::Sample sample{ measured.mean(), measured.last(), measured.steps(), {} };
    for (const v3d::engine::Statistics::Row& row : measured.rows()) {
        sample.spans.push_back({ row.name, row.mean });
    }
    return sample;
}

/**
 **/
void Shell::draw(const v3d::ui::shell::StatisticsOverlay::Sample& sample) {
    if (screen_) {
        screen_->draw(ui_.get(), sample);
    }
}

/**
 **/
void Shell::draw() {
    draw(sample());
}

};  // namespace v3d::shell
