/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Screen.h"

#include <api/render/realtime/Engine3D.h>

#include <cstdint>

#include <boost/make_shared.hpp>
#include <glm/vec2.hpp>

namespace v3d::ui::shell {

namespace {

/**
 * The dressing an app gets when it names none: a line of text and the gap after it.
 **/
void lineHeight(paint::Dressing* dressing, float size) {
    dressing->lineHeight = size * 1.4f;
}

};  // namespace

/**
 **/
Screen::Screen(v3d::render::realtime::Engine3D* engine, const boost::shared_ptr<v3d::asset::Manager>& assets,
    const boost::shared_ptr<v3d::log::Logger>& logger, const Options& options) :
    engine_(engine),
    options_(options),
    scale_(1.0f),
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
    const float drawn = size();
    components_ = boost::make_shared<paint::ComponentRenderer>(text_->measure(drawn), text_->write(&canvas_, drawn));
    options_.dress(&components_->dressing(), drawn);
    components_->theme(theme_);

    if (options_.immediate) {
        // built once and given the new text after that, so a rescale keeps where its windows
        // were dragged, what is folded and how far each list is scrolled
        if (immediate_) {
            immediate_->text(text_->measure(drawn), text_->write(&canvas_, drawn));
        } else {
            immediate_ = boost::make_shared<Immediate>(text_->measure(drawn), text_->write(&canvas_, drawn));
        }
        // the immediate layer has a dressing of its own kind, which the app's dress does not
        // fill, so only the line height is carried across
        immediate_->dressing().lineHeight = components_->dressing().lineHeight;
        immediate_->theme(theme_);
    }
    if (statistics_) {
        statistics_->size(StatisticsOverlay::defaultSize * scale_);
    }
}

/**
 **/
bool Screen::begin() {
    resized_ = false;
    glm::ivec2 frame(0, 0);
    if (engine_ == nullptr || !engine_->beginFrame(&frame)) {
        return false;
    }
    const auto width = static_cast<std::uint32_t>(frame.x);
    const auto height = static_cast<std::uint32_t>(frame.y);
    if (canvas_.width() != width || canvas_.height() != height) {
        canvas_.resize(width, height);
        resized_ = true;
    }
    canvas_.clear();
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
    if (factor <= 0.0f || factor == scale_) {
        return;
    }
    scale_ = factor;
    build();
}

/**
 **/
float Screen::scale() const noexcept {
    return scale_;
}

/**
 **/
float Screen::size() const noexcept {
    return options_.size * scale_;
}

/**
 **/
void Screen::theme(const boost::shared_ptr<style::Theme>& theme) {
    theme_ = theme;
    components_->theme(theme_);
    if (immediate_) {
        immediate_->theme(theme_);
    }
}

/**
 **/
void Screen::draw(const Engine* ui, const StatisticsOverlay::Sample& statistics) {
    if (ui != nullptr) {
        components_->draw(&canvas_, *ui);
    }
    // last, so the numbers sit over an open menu rather than under it
    if (statistics_) {
        statistics_->draw(&canvas_, statistics);
    }
}

/**
 **/
v3d::render::realtime::Canvas& Screen::canvas() noexcept {
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
