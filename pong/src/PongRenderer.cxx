/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongRenderer.h"

#include <GL/glew.h>

#include <cmath>
#include <iostream>
#include <string>

#include "../../api/gl/Shader.h"
#include "../../api/font/TextureTextBuffer.h"
#include "../../api/asset/ShaderProgram.h"
#include "../../api/asset/TextureFont.h"
#include "../../api/render/realtime/Frame.h"
#include "../../api/render/realtime/operation/Canvas.h"

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

PongRenderer::PongRenderer(const boost::shared_ptr<v3d::render::realtime::Window3D>& window, const boost::shared_ptr<v3d::log::Logger>& logger, const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    engine_(logger, assetManager, registry) {
    engine_.initialize(window);

    boost::shared_ptr<v3d::asset::Loader> loader = assetManager->resolveLoader(v3d::asset::Type::ShaderProgram);
    v3d::asset::ParameterValue value;
    value = static_cast<unsigned int>(v3d::gl::Shader::SHADER_TYPE_VERTEX | v3d::gl::Shader::SHADER_TYPE_FRAGMENT);
    loader->parameter("shaderTypes", value);
    boost::shared_ptr<v3d::asset::ShaderProgram> program = boost::dynamic_pointer_cast<v3d::asset::ShaderProgram>(
        assetManager->load("shaders/canvas", v3d::asset::Type::ShaderProgram));
    canvasProgram_ = program->program();

    canvas_ = boost::make_shared<v3d::gl::Canvas>();

    // setup text buffer
    boost::shared_ptr<v3d::asset::ShaderProgram> textProgram = boost::dynamic_pointer_cast<v3d::asset::ShaderProgram>(
        assetManager->load("shaders/text", v3d::asset::Type::ShaderProgram));

    fontCache_ = boost::make_shared<v3d::font::TextureFontCache>(512, 512, v3d::font::TextureTextBuffer::LCD_FILTERING_ON, logger);

    markup_.bold_ = false;
    markup_.italic_ = false;
    markup_.rise_ = 0.0f;
    markup_.spacing_ = 0.0f;
    markup_.gamma_ = 0.5f;
    markup_.underline_ = false;
    markup_.overline_ = false;
    markup_.strikethrough_ = false;
    markup_.foregroundColor_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    markup_.backgroundColor_ = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f);
    markup_.size_ = 64.0f;

    // characters to cache
    const wchar_t *charcodes =  L" !\"#$%&'()*+,-./0123456789:;<=>?"
                                L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
                                L"`abcdefghijklmnopqrstuvwxyz{|}~";
    fontCache_->charcodes(charcodes);

    loader->parameter("fontSize", markup_.size_);
    boost::shared_ptr<v3d::asset::TextureFont> font = boost::dynamic_pointer_cast<v3d::asset::TextureFont>(
        assetManager->load("fonts/DroidSerif-Regular.ttf", v3d::asset::Type::TextureFont));
    font->font()->atlas(fontCache_->atlas());
    font->font()->loadGlyphs(charcodes);
    fontCache_->add(font->font());
    markup_.font_ = font->font();

    boost::shared_ptr<v3d::font::TextureTextBuffer> text;
    text = boost::make_shared<v3d::font::TextureTextBuffer>();
    fontRenderer_ = boost::make_shared<v3d::render::realtime::operation::TextureFont>(text, textProgram->program(), fontCache_->atlas(), logger);
}

void PongRenderer::resize(int width, int height) {
    scene_->resize(width, height);
    canvas_->resize(width, height);
    canvasProgram_->enable();
    const float w = static_cast<float>(width);
    const float h = static_cast<float>(height);
    glm::mat4 projection = glm::ortho(0.0f, w, h, 0.0f, -1.0f, 1.0f);
    unsigned int projectionMatrix = canvasProgram_->uniform("projectionMatrix");
    glUniformMatrix4fv(projectionMatrix, 1, GL_FALSE, glm::value_ptr(projection));
    canvasProgram_->disable();
    fontRenderer_->resize(w, h);
}

void PongRenderer::draw() {
    canvas_->clear();

    engine_.renderFrame();

    const int width = engine_.window()->width();
    const int height = engine_.window()->height();

    // draw the scoreboard
    fontRenderer_->buffer()->clear();
    std::stringstream txt;

    // left score
    txt << boost::lexical_cast<std::string>(scene_->left().score());
    glm::vec2 pen(width / 4.0f, height / 4.0f);
    std::string buffer = txt.str();
    std::wstring widestr = std::wstring(buffer.begin(), buffer.end());
    fontRenderer_->buffer()->addText(&pen, markup_, widestr.c_str());

    // right score
    txt.str("");
    txt.clear();
    pen = glm::vec2(((width / 4.0f) * 3.0f), (height / 4.0f));
    txt << boost::lexical_cast<std::string>(scene_->right().score());
    buffer = txt.str();
    widestr = std::wstring(buffer.begin(), buffer.end());
    fontRenderer_->buffer()->addText(&pen, markup_, widestr.c_str());

    // draw the gameboard

    // center line
    glm::vec3 color(0.35f, 0.35f, 0.35f);
    canvas_->rect(((width / 2) - 7), ((width / 2) + 7), 0, height, color);

    // bottom wall
    canvas_->rect(0, width, height - 15, height, color);

    // top wall
    canvas_->rect(0, width, 0, 15, color);

    // draw the paddles and ball
    drawPaddle(scene_->left());
    drawPaddle(scene_->right());

    drawBall();

    v3d::render::realtime::Frame frame(engine_.context());
    boost::shared_ptr<v3d::render::realtime::Operation> canvasOp = boost::make_shared<v3d::render::realtime::operation::Canvas>(canvas_, nullptr);
    frame.addOperation(canvasOp);
    frame.addOperation(fontRenderer_);

    // upload to GPU & render
    frame.draw();
}

void PongRenderer::drawBall() {
    const int sides = 32;
    glm::vec2 position = scene_->ball().position();

    canvas_->push();
    canvas_->translate(position);
    glm::vec3 color(1.0f, 1.0f, 1.0f);
    canvas_->circle(sides, static_cast<size_t>(scene_->ball().size()), color);

    canvas_->pop();
}

void PongRenderer::drawPaddle(const Paddle & paddle) {
    canvas_->push();
    glm::vec3 color = paddle.color();
    // use a translation to get to object space
    // offsetting y so our origin is the left corner
    // x is adjusted depending on which side of the screen the paddle is on
    canvas_->translate(glm::vec2(paddle.offset(), paddle.position() - 25));
    canvas_->rect(0, 15, 0, 50, color);
    canvas_->pop();
}
