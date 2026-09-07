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
    const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    engine_(logger, assetManager, registry) {
    engine_.initialize(window);

    text_ = boost::make_shared<v3d::ui::TextRenderer>(assetManager, logger, engine_.quads());

    statistics_ = boost::make_shared<v3d::ui::StatisticsOverlay>(text_);

    uiRenderer_ = boost::make_shared<v3d::ui::ComponentRenderer>(text_->measure(fontSize), text_->write(&canvas_, fontSize));
    uiRenderer_->dressing().lineHeight = fontSize * 1.4f;
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
const boost::shared_ptr<v3d::ui::StatisticsOverlay>& PongRenderer::statistics() const {
    return statistics_;
}

/**
 **/
void PongRenderer::shutdown() {
    engine_.shutdown();
}

/**
 **/
void PongRenderer::resize(int width, int height) {
    if (scene_) {
        scene_->resize(width, height);
    }
    canvas_.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

/**
 **/
void PongRenderer::draw(const v3d::ui::StatisticsOverlay::Sample& statistics) {
    if (!scene_) {
        return;
    }

    glm::ivec2 size;
    if (!engine_.beginFrame(&size)) {
        return;
    }
    if (canvas_.width() != static_cast<uint32_t>(size.x) || canvas_.height() != static_cast<uint32_t>(size.y)) {
        resize(size.x, size.y);
    }

    canvas_.clear();

    drawBoard();
    drawPaddle(scene_->left());
    drawPaddle(scene_->right());
    drawBall();
    drawScores();

    if (ui_) {
        uiRenderer_->draw(&canvas_, *ui_);
    }

    // last, so the numbers sit over the menu as well as the game
    statistics_->draw(&canvas_, statistics);

    boost::shared_ptr<v3d::render::realtime::Pass> pass =
        engine_.frame()->pass(v3d::render::realtime::Engine3D::colourPass);
    engine_.quads()->submit(canvas_, pass.get());

    engine_.renderFrame();
}

/**
 **/
void PongRenderer::drawBoard() {
    const float width = static_cast<float>(engine_.window()->width());
    const float height = static_cast<float>(engine_.window()->height());
    const float half = centreLineWidth * 0.5f;
    const float wall = static_cast<float>(wallThickness);

    // centre line
    canvas_.rect(glm::vec2(width * 0.5f - half, 0.0f), glm::vec2(width * 0.5f + half, height), boardColour);
    // top and bottom walls
    canvas_.rect(glm::vec2(0.0f, 0.0f), glm::vec2(width, wall), boardColour);
    canvas_.rect(glm::vec2(0.0f, height - wall), glm::vec2(width, height), boardColour);
}

/**
 **/
void PongRenderer::drawScores() {
    const float width = static_cast<float>(engine_.window()->width());
    const float height = static_cast<float>(engine_.window()->height());

    const std::string left = boost::lexical_cast<std::string>(scene_->left().score());
    const std::string right = boost::lexical_cast<std::string>(scene_->right().score());

    text_->draw(&canvas_, left, glm::vec2(width * 0.25f, height * 0.25f), scoreColour, fontSize);
    text_->draw(&canvas_, right, glm::vec2(width * 0.75f, height * 0.25f), scoreColour, fontSize);
}

/**
 **/
void PongRenderer::drawBall() {
    canvas_.circle(scene_->ball().position(), scene_->ball().size(), ballSides, ballColour);
}

/**
 **/
void PongRenderer::drawPaddle(const Paddle& paddle) {
    canvas_.push();
    // the paddle's position is the centre of its travel; its rectangle is drawn from the corner
    canvas_.translate(glm::vec2(paddle.offset(), paddle.position() - 25.0f));
    const glm::vec3 colour = paddle.color();
    canvas_.rect(glm::vec2(0.0f, 0.0f), glm::vec2(15.0f, 50.0f), glm::vec4(colour, 1.0f));
    canvas_.pop();
}
