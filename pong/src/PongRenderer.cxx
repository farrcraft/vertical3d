/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongRenderer.h"

#include <GL/glew.h>

#include <cmath>
#include <iostream>
#include <string>

#include "../../v3dlibs/gl/Shader.h"
#include "../../v3dlibs/font/TextureTextBuffer.h"

#include "../../stark/ProgramFactory.h"
#include "../../stark/AssetLoader.h"

#include <boost/lexical_cast.hpp>


PongRenderer::PongRenderer(boost::shared_ptr<PongScene> scene, boost::shared_ptr<AssetLoader> & loader) : scene_(scene), fonts_(new v3d::FontCache()) {
    ProgramFactory factory(loader);
    boost::shared_ptr<v3d::Program> program = factory.create(v3d::Shader::SHADER_TYPE_VERTEX|v3d::Shader::SHADER_TYPE_FRAGMENT, "shaders/canvas");

    canvas_.reset(new v3d::Canvas(program));

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_TRUE);
    glDepthFunc(GL_LEQUAL);
    glDepthRange(0.0f, 1.0f);
    glEnable(GL_DEPTH_CLAMP);
    // CCW winding is default
    glFrontFace(GL_CCW);
    glActiveTexture(GL_TEXTURE0);

    // setup text buffer
    boost::shared_ptr<v3d::Program> textProgram = factory.create(v3d::Shader::SHADER_TYPE_VERTEX|v3D::Shader::SHADER_TYPE_FRAGMENT, "shaders/text");
    fontCache_.reset(new v3d::TextureFontCache(512, 512, v3d::TextureTextBuffer::LCD_FILTERING_ON));

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

    std::string filename = loader->path() + std::string("fonts/DroidSerif-Regular.ttf");
    markup_.font_ = fontCache_->load(filename, markup_.size_);

    boost::shared_ptr<v3d::TextureTextBuffer> text;
    text.reset(new v3d::TextureTextBuffer());
    fontRenderer_.reset(new v3d::TextureFontRenderer(text, textProgram, fontCache_->atlas()));
}

boost::shared_ptr<v3d::FontCache> PongRenderer::fonts() const {
    return fonts_;
}

void PongRenderer::resize(int width, int height) {
    scene_->resize(width, height);
    canvas_->resize(width, height);
    float w = static_cast<float>(width);
    float h = static_cast<float>(height);
    fontRenderer_->resize(w, h);
}

void PongRenderer::draw(Hookah::Window * window) {
    canvas_->clear();

    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    int width = window->width();
    int height = window->height();

    // draw the scoreboard
    fontRenderer_->buffer()->clear();
    std::stringstream txt;

    // left score
    txt << boost::lexical_cast<std::string>(scene_->left().score());
    glm::vec2 pen(width / 4.0f, height / 4.0f);
    std::string buffer = txt.str();
    std::wstring widestr = std::wstring(buffer.begin(), buffer.end());
    fontRenderer_->buffer()->addText(pen, markup_, widestr.c_str());

    // right score
    txt.str("");
    txt.clear();
    pen = glm::vec2(((width / 4.0f) * 3.0f), (height / 4.0f));
    txt << boost::lexical_cast<std::string>(scene_->right().score());
    buffer = txt.str();
    widestr = std::wstring(buffer.begin(), buffer.end());
    fontRenderer_->buffer()->addText(pen, markup_, widestr.c_str());

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

    // upload to GPU & render
    canvas_->upload();
    fontRenderer_->upload();

    canvas_->render();
    fontRenderer_->render();
}

void PongRenderer::drawBall() {
    int sides = 32;
    glm::vec2 position = scene_->ball().position();

    canvas_->push();
    canvas_->translate(position);
    glm::vec3 color(1.0f, 1.0f, 1.0f);
    canvas_->circle(sides, scene_->ball().size(), color);

    canvas_->pop();
}

void PongRenderer::drawPaddle(const Paddle & paddle) {
    canvas_->push();
    glm::vec3 color1 = paddle.color1();
    glm::vec3 color2 = paddle.color2();
    // use a translation to get to object space
    // offsetting y so our origin is the left corner
    // x is adjusted depending on which side of the screen the paddle is on
    canvas_->translate(glm::vec2(paddle.offset(), paddle.position() - 25));
    canvas_->rect(0, 15, 0, 50, color1);
    canvas_->pop();
}
