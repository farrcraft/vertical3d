/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Screen.h"

#include <api/asset/Manager.h>
#include <api/image/Image.h>
#include <api/log/Logger.h>
#include <api/render/realtime/WindowRenderer.h>
#include <api/ui/Component.h>
#include <api/ui/Interface.h>
#include <api/ui/Transition.h>
#include <api/ui/input/Command.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/paint/Dressing.h>
#include <api/ui/paint/Text.h>
#include <api/ui/paint/TextRenderer.h>

#include "StatisticsOverlay.h"

#include <chrono>
#include <cstdint>
#include <string>
#include <utility>

#include <boost/make_shared.hpp>
#include <glm/vec2.hpp>

namespace v3d::ui::shell {

namespace {

/**
 * The height of a line of text, as a multiple of the text's size.
 **/
constexpr float LINE_SPACING = 1.4F;

/**
 * The Dress used when an app passes none. It sets only the line height, which covers a line
 * of text and the gap after it.
 **/
void lineHeight(paint::Dressing* dressing, float size) {
    dressing->lineHeight = size * LINE_SPACING;
}

};  // namespace

/**
 **/
Screen::Screen(v3d::render::realtime::WindowRenderer* engine, const boost::shared_ptr<v3d::asset::Manager>& assets,
    const boost::shared_ptr<v3d::log::Logger>& logger, const Options& options) :
    engine_(engine),
    options_(options),
    scale_(1.0F),
    display_(1.0F),
    built_(0.0F),
    resized_(false) {
    if (!options_.dress) {
        options_.dress = lineHeight;
    }

    // the atlas is a texture on the device, and this upload is the only line here that
    // needs one
    text_ = boost::make_shared<paint::TextRenderer>(assets, logger,
        [engine](const boost::shared_ptr<v3d::image::Image>& atlas) {
            if (engine == nullptr || !engine->textures()) {
                return v3d::render::realtime::TextureHandle();
            }
            return engine->textures()->texture(atlas);
        });
    if (options_.statistics) {
        statistics_ = boost::make_shared<StatisticsOverlay>(text_);
        if (engine != nullptr && engine->context()) {
            statistics_->device(engine->context()->device()->description().name);
        }
    }
    build();
}

/**
 **/
void Screen::build() {
    // the theme lives on the Interface, and the renderer being replaced is what was last given it
    const boost::shared_ptr<style::Theme> drawing = components_ ? components_->theme() : boost::shared_ptr<style::Theme>();
    built_ = factor();
    const float drawn = size();
    components_ = boost::make_shared<paint::ComponentRenderer>(text_->measure(drawn), text_->write(&canvas_, drawn));
    // the built-in sizes at this scale, which the app's dress then fills in from the text size,
    // and a theme's sizes at the same scale
    components_->dressing() = paint::Dressing().scaled(built_);
    components_->scale(built_);
    components_->shortcuts(options_.shortcut);
    options_.dress(&components_->dressing(), drawn);

    if (options_.immediate) {
        // built once and given the new text after that, so a rescale keeps where its windows
        // were dragged, what is folded and how far each list is scrolled
        if (immediate_) {
            immediate_->text(text_->measure(drawn), text_->write(&canvas_, drawn));
        } else {
            immediate_ = boost::make_shared<Immediate>(text_->measure(drawn), text_->write(&canvas_, drawn));
        }
        immediate_->titles(text_->write(&canvas_, drawn, paint::Weight::SemiBold));
        immediate_->scale(built_);
        // the app's Dress fills in a hud's sizes, and they are larger than a tool panel's. Only
        // the line height follows the text, so only it is copied across
        immediate_->dressing().lineHeight = components_->dressing().lineHeight;
    }
    theme(drawing);
    if (statistics_) {
        statistics_->size(StatisticsOverlay::defaultSize * built_);
    }
}

/**
 **/
bool Screen::begin(const Interface* ui) {
    if (ui != nullptr && ui->activeTheme() != components_->theme()) {
        theme(ui->activeTheme());
    }
    resized_ = false;
    glm::ivec2 frame(0, 0);
    if (engine_ == nullptr || !engine_->beginFrame(&frame)) {
        return false;
    }
    const auto width = static_cast<std::uint32_t>(frame.x);
    const auto height = static_cast<std::uint32_t>(frame.y);
    if (canvas_.width() != width || canvas_.height() != height) {
        canvas_.resize(glm::uvec2(width, height));
        resized_ = true;
        // a screen that follows the window's height is drawn at a new size
        if (factor() != built_) {
            build();
        }
    }
    canvas_.clear();

    // real time rather than the simulation's, so a menu still eases open over a paused game.
    // The first frame starts the clock at zero
    const std::chrono::steady_clock::time_point now = std::chrono::steady_clock::now();
    if (clock_.frame > 0) {
        clock_.now += std::chrono::duration<float>(now - last_).count();
    }
    last_ = now;
    clock_.frame++;
    components_->clock(clock_);
    if (immediate_) {
        immediate_->clock(clock_);
    }
    return true;
}

/**
 **/
bool Screen::resized() const noexcept {
    return resized_;
}

/**
 **/
void Screen::scale(float factor) {
    if (factor <= 0.0F || factor == scale_) {
        return;
    }
    scale_ = factor;
    build();
}

/**
 **/
void Screen::display(float monitor) {
    if (monitor <= 0.0F || monitor == display_) {
        return;
    }
    display_ = monitor;
    if (factor() != built_) {
        build();
    }
}

/**
 **/
const Clock& Screen::clock() const noexcept {
    return clock_;
}

/**
 **/
float Screen::factor() const noexcept {
    if (options_.scaling == Scaling::Display) {
        return scale_ * display_;
    }
    // before the first frame the canvas has no height, and the screen is drawn at its own scale
    if (canvas_.height() == 0 || options_.reference <= 0.0F) {
        return scale_;
    }
    return scale_ * (static_cast<float>(canvas_.height()) / options_.reference);
}

/**
 **/
float Screen::scale() const noexcept {
    return scale_;
}

/**
 **/
float Screen::size() const noexcept {
    return options_.size * factor();
}

/**
 **/
void Screen::theme(const boost::shared_ptr<style::Theme>& theme) {
    components_->theme(theme);
    if (immediate_) {
        immediate_->theme(theme);
    }
}

/**
 **/
void Screen::draw(const Interface* ui, const StatisticsOverlay::Sample& statistics) {
    if (ui != nullptr) {
        components_->draw(&canvas_, *ui);
    }
    // last, so the numbers sit over an open menu rather than under it
    if (statistics_) {
        statistics_->draw(&canvas_, statistics);
    }
    // and a tooltip over everything, since it is about whatever is under it
    if (ui != nullptr) {
        tip(*ui);
    }
}

/**
 **/
void Screen::tip(const Interface& ui) {
    const boost::shared_ptr<Component> under = ui.hovered();
    // read without writing, which would drop what the renderer has resolved
    const paint::Dressing& dress = std::as_const(*components_).dressing();
    if (!tooltip_.due(under, clock_.now, {.first = dress.tooltipDelay, .following = dress.tooltipQuickDelay})) {
        return;
    }
    components_->tooltip(&canvas_, std::string(under->tooltip()), components_->shortcut(input::command(under)), under->bound());
}

/**
 **/
v3d::render::realtime::canvas::Canvas& Screen::canvas() noexcept {
    return canvas_;
}

/**
 **/
const boost::shared_ptr<paint::TextRenderer>& Screen::text() const noexcept {
    return text_;
}

/**
 **/
paint::ComponentRenderer& Screen::components() noexcept {
    return *components_;
}

/**
 **/
const boost::shared_ptr<StatisticsOverlay>& Screen::statistics() const noexcept {
    return statistics_;
}

/**
 **/
Immediate* Screen::immediate() noexcept {
    return immediate_.get();
}

};  // namespace v3d::ui::shell
