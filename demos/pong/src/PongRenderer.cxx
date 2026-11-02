/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongRenderer.h"

#include <api/log/Logger.h>
#include <api/render/realtime/frame/Pass.h>
#include <api/shell/Shell.h>
#include <api/ui/paint/ComponentRenderer.h>
#include <api/ui/shell/Screen.h>
#include <api/window/Window.h>

#include "PongScene.h"

#include <string>

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

/**
 **/
PongRenderer::PongRenderer(const boost::shared_ptr<v3d::window::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    v3d::shell::Shell* shell) :
    shell_(shell),
    engine_(logger) {
    engine_.initialize(window);
    engine_.clearColour(barColour);

    // the rules are written in the court's units, and the court is fitted to the window
    court_.space(glm::vec2(PongScene::width, PongScene::height), v3d::render::realtime::canvas::Canvas::Fit::Contain);

    // after the engine is initialized, because the screen's atlas is uploaded through it
    shell_->present(&engine_, v3d::shell::Shell::game());
}

/**
 **/
void PongRenderer::scene(const boost::shared_ptr<PongScene>& scene) {
    scene_ = scene;
}

/**
 **/
void PongRenderer::shutdown() {
    engine_.shutdown();
}

/**
 **/
void PongRenderer::draw(float alpha) {
    v3d::ui::shell::Screen* screen = shell_->screen();
    if (!scene_ || screen == nullptr) {
        return;
    }

    if (!screen->begin(shell_->ui().get())) {
        return;
    }
    const v3d::render::realtime::canvas::Canvas& ui = screen->canvas();
    court_.resize(glm::uvec2(ui.width(), ui.height()));
    court_.clear();

    drawBoard();
    drawPaddle(scene_->left(), alpha);
    drawPaddle(scene_->right(), alpha);
    drawBall(alpha);
    drawScores();

    shell_->draw();

    // the screen's canvas is a layer of its own, in front of the court
    const boost::shared_ptr<v3d::render::realtime::frame::Pass> pass =
        engine_.frame()->pass(v3d::render::realtime::WindowRenderer::colourPass);
    engine_.quads()->submit(court_, pass.get(), 0);
    engine_.quads()->submit(ui, pass.get(), 1);

    engine_.renderFrame();
}

/**
 **/
void PongRenderer::drawBoard() {
    const float width = PongScene::width;
    const float height = PongScene::height;
    const float half = centreLineWidth * 0.5F;
    const float wall = PongScene::wall;

    court_.rect(glm::vec2(0.0F, 0.0F), glm::vec2(width, height), courtColour);

    // centre line
    court_.rect(glm::vec2((width * 0.5F) - half, 0.0F), glm::vec2((width * 0.5F) + half, height), boardColour);
    // top and bottom walls
    court_.rect(glm::vec2(0.0F, 0.0F), glm::vec2(width, wall), boardColour);
    court_.rect(glm::vec2(0.0F, height - wall), glm::vec2(width, height), boardColour);
}

/**
 **/
void PongRenderer::drawScores() {
    const float width = PongScene::width;
    const float height = PongScene::height;

    const std::string left = boost::lexical_cast<std::string>(scene_->left().score());
    const std::string right = boost::lexical_cast<std::string>(scene_->right().score());

    // drawn over the court in the theme's text, centred on a point of the court, so each score
    // stays over its half whatever the window's shape
    v3d::ui::shell::Screen* screen = shell_->screen();
    const glm::vec2 centre(0.5F, 0.5F);
    screen->components().hud(&screen->canvas(), { left },
        { .at = court_.toPixels(glm::vec2(width * leftScoreColumn, height * scoreRow)), .anchor = centre });
    screen->components().hud(&screen->canvas(), { right },
        { .at = court_.toPixels(glm::vec2(width * rightScoreColumn, height * scoreRow)), .anchor = centre });
}

/**
 **/
void PongRenderer::drawBall(float alpha) {
    // size is the side of the box the scene collides, so it is the circle's diameter
    court_.circle(scene_->ball().drawn(alpha), scene_->ball().size() / 2.0F, ballSides, ballColour);
}

/**
 **/
void PongRenderer::drawPaddle(const Paddle& paddle, float alpha) {
    court_.push();
    // the paddle's position is the centre of its length. The rectangle is drawn from its corner
    court_.translate(glm::vec2(paddle.offset(), paddle.drawn(alpha) - (paddle.length() / 2.0F)));
    const glm::vec3 colour = paddle.color();
    court_.rect(glm::vec2(0.0F, 0.0F), glm::vec2(paddle.size(), paddle.length()), glm::vec4(colour, 1.0F));
    court_.pop();
}
