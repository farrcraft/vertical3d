/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Shell.h"

#include <api/asset/kind/Json.h>
#include <api/config/Config.h>
#include <api/config/Type.h>
#include <api/engine/Engine.h>
#include <api/engine/Statistics.h>
#include <api/ui/Engine.h>
#include <api/ui/shell/GameMenu.h>
#include <api/ui/shell/Screen.h>
#include <api/ui/shell/StatisticsOverlay.h>

#include <boost/make_shared.hpp>

namespace v3d::shell {

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
    ui_ = boost::make_shared<v3d::ui::Engine>(engine_->events(), engine_->dispatcher(), engine_->logger());
    if (options_.menu) {
        menu_ = boost::make_shared<v3d::ui::shell::GameMenu>(ui_, suspend);
    }
    const boost::shared_ptr<v3d::config::Config>& config = engine_->config();
    const boost::shared_ptr<v3d::asset::kind::Json> document =
        config ? config->get(v3d::config::Type::Ui) : boost::shared_ptr<v3d::asset::kind::Json>();
    return !document || ui_->load(document->document());
}

/**
 **/
void Shell::present(v3d::render::realtime::Engine3D* renderer, const v3d::ui::shell::Screen::Options& options) {
    screen_ = boost::make_shared<v3d::ui::shell::Screen>(renderer, engine_->assets(), engine_->logger(), options);
}

/**
 **/
void Shell::release() {
    screen_.reset();
    menu_.reset();
    ui_.reset();
}

/**
 **/
const boost::shared_ptr<v3d::ui::Engine>& Shell::ui() const noexcept {
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
