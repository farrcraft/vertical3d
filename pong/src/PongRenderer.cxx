/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongRenderer.h"

#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * The size the ui and the scores are drawn at, which the one atlas is scaled to per
 * ADR-0036 rather than rasterized at.
 **/
const float fontSize = 28.0f;

// the court is drawn over black, so a window of another shape shows where it ends
constexpr glm::vec4 barColour(0.0f, 0.0f, 0.0f, 1.0f);
constexpr glm::vec4 courtColour(0.06f, 0.07f, 0.10f, 1.0f);
constexpr glm::vec4 boardColour(0.35f, 0.35f, 0.35f, 1.0f);
constexpr glm::vec4 ballColour(1.0f, 1.0f, 1.0f, 1.0f);
constexpr glm::vec4 scoreColour(0.85f, 0.85f, 0.85f, 1.0f);

const unsigned int ballSides = 32;
const unsigned int wallThickness = 15;
const unsigned int centreLineWidth = 14;

};  // namespace

/**
 **/
PongRenderer::PongRenderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager) :
    engine_(logger, assetManager) {
    engine_.initialize(window);
    engine_.clearColour(barColour);

    // the rules are written in the court's units, and the court is fitted to the window
    court_.space(glm::vec2(PongScene::width, PongScene::height), v3d::render::realtime::Canvas::Fit::Contain);

    v3d::ui::shell::Screen::Options options;
    options.size = fontSize;
    screen_ = boost::make_shared<v3d::ui::shell::Screen>(&engine_, assetManager, logger, options);
}

/**
 **/
void PongRenderer::scene(const boost::shared_ptr<PongScene>& scene) {
    scene_ = scene;
}

/**
 **/
void PongRenderer::ui(const boost::shared_ptr<v3d::ui::Engine>& ui) {
    ui_ = ui;
}

/**
 **/
const boost::shared_ptr<v3d::ui::shell::StatisticsOverlay>& PongRenderer::statistics() const {
    return screen_->statistics();
}

/**
 **/
void PongRenderer::shutdown() {
    engine_.shutdown();
}

/**
 **/
void PongRenderer::draw(const v3d::ui::shell::StatisticsOverlay::Sample& statistics, float alpha) {
    if (!scene_) {
        return;
    }

    if (!screen_->begin()) {
        return;
    }
    const v3d::render::realtime::Canvas& ui = screen_->canvas();
    court_.resize(ui.width(), ui.height());
    court_.clear();

    drawBoard();
    drawPaddle(scene_->left(), alpha);
    drawPaddle(scene_->right(), alpha);
    drawBall(alpha);
    drawScores();

    screen_->draw(ui_.get(), statistics);

    // the menu in front of the court, as a layer of its own
    boost::shared_ptr<v3d::render::realtime::Pass> pass =
        engine_.frame()->pass(v3d::render::realtime::Engine3D::colourPass);
    engine_.quads()->submit(court_, pass.get(), 0);
    engine_.quads()->submit(ui, pass.get(), 1);

    engine_.renderFrame();
}

/**
 **/
void PongRenderer::drawBoard() {
    const float width = PongScene::width;
    const float height = PongScene::height;
    const float half = centreLineWidth * 0.5f;
    const float wall = static_cast<float>(wallThickness);

    court_.rect(glm::vec2(0.0f, 0.0f), glm::vec2(width, height), courtColour);

    // centre line
    court_.rect(glm::vec2(width * 0.5f - half, 0.0f), glm::vec2(width * 0.5f + half, height), boardColour);
    // top and bottom walls
    court_.rect(glm::vec2(0.0f, 0.0f), glm::vec2(width, wall), boardColour);
    court_.rect(glm::vec2(0.0f, height - wall), glm::vec2(width, height), boardColour);
}

/**
 **/
void PongRenderer::drawScores() {
    const float width = PongScene::width;
    const float height = PongScene::height;

    const std::string left = boost::lexical_cast<std::string>(scene_->left().score());
    const std::string right = boost::lexical_cast<std::string>(scene_->right().score());

    screen_->text()->draw(&court_, left, glm::vec2(width * 0.25f, height * 0.25f), scoreColour, fontSize);
    screen_->text()->draw(&court_, right, glm::vec2(width * 0.75f, height * 0.25f), scoreColour, fontSize);
}

/**
 **/
void PongRenderer::drawBall(float alpha) {
    court_.circle(scene_->ball().drawn(alpha), scene_->ball().size(), ballSides, ballColour);
}

/**
 **/
void PongRenderer::drawPaddle(const Paddle& paddle, float alpha) {
    court_.push();
    // the paddle's position is the centre of its travel; its rectangle is drawn from the corner
    court_.translate(glm::vec2(paddle.offset(), paddle.drawn(alpha) - 25.0f));
    const glm::vec3 colour = paddle.color();
    court_.rect(glm::vec2(0.0f, 0.0f), glm::vec2(15.0f, 50.0f), glm::vec4(colour, 1.0f));
    court_.pop();
}
