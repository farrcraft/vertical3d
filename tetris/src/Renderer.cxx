/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <GL/glew.h>

#include <iostream>
#include <string>

#include "Renderer.h"
#include "TetrisScene.h"

#include "../../api/image/Factory.h"
#include "../../api/gl/GLFontRenderer.h"

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

TetrisRenderer::TetrisRenderer(boost::shared_ptr<TetrisScene> scene, const boost::shared_ptr<v3d::log::Logger> & logger) :
    scene_(scene), fonts_(new v3d::font::FontCache(logger)), logger_(logger) {
    // load a font to use for debugging output
    fonts_->load("debug", "/usr/share/fonts/corefonts/arial.ttf", 32);

    try {
        v3d::image::Factory factory(logger);
        // load textures
        boost::shared_ptr<v3d::gl::GLTexture> texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/red.tga"));
        textures_["red"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/blue.tga"));
        textures_["blue"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/cyan.tga"));
        textures_["cyan"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/green.tga"));
        textures_["green"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/orange.tga"));
        textures_["orange"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/purple.tga"));
        textures_["purple"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/yellow.tga"));
        textures_["yellow"] = texture;
    }
    catch (...) {
        LOG_ERROR(logger) << "texture load exception!" << std::endl;
    }
    glShadeModel(GL_SMOOTH);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
    glClearDepth(1.0f);
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
    glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_NICEST);
}

void TetrisRenderer::resize(int width, int height) {
    scene_->resize(width, height);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    // map top left to (0,0)
    glOrtho(0, static_cast<float>(width), static_cast<float>(height), 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
}

void TetrisRenderer::draw(Hookah::Window * window) {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glPushAttrib(GL_TEXTURE_BIT);
    // flood fill the entire window with dark grey
    glDisable(GL_TEXTURE_2D);
    // glBindTexture(GL_TEXTURE_2D, 0);
    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(window->width(), 0);
        glVertex2i(window->width(), window->height());
        glVertex2i(0, window->height());
    glEnd();

    // draw the game board
    drawBoard();
    glPopAttrib();
}

bool TetrisRenderer::drawBoard() {
    float quad_size = 25.0f;
    GameBoard * board = scene_->board();

    // draw the board area with a black background in the center of the window
    glColor3f(0.0f, 0.0f, 0.0f);
    glTranslatef(quad_size * board->rows() / 2.0f, quad_size * board->columns() / 4.0f, 0.0f);
    glBegin(GL_QUADS);
        glVertex2f(0.0f, 0.0f);
        glVertex2f(quad_size * board->columns(), 0.0f);
        glVertex2f(quad_size * board->columns(), quad_size * board->rows());
        glVertex2f(0.0f, quad_size * board->rows());
    glEnd();

    // draw existing pieces
    glEnable(GL_TEXTURE_2D);
    glColor3f(1.0f, 1.0f, 1.0f);

    float x_offset = 0.0f;
    float y_offset = 0.0f;
    for (unsigned int i = 0; i < board->rows(); i++) {
        for (unsigned int j = 0; j < board->columns(); j++) {
            glPushMatrix();
            glTranslatef(x_offset, y_offset, 0.0f);
            glScalef(quad_size, quad_size, quad_size);
            drawPiece(board->piece(j, i));
            x_offset += quad_size;
            glPopMatrix();
        }
        y_offset += quad_size;
        x_offset = 0.0f;
    }

    // draw falling tetrad
    drawTetrad(board->currentTetrad(), board->debug());

    // draw next tetrad
    glPushMatrix();
    x_offset = quad_size * (board->columns() + 4);
    y_offset = quad_size * 2;
    glTranslatef(x_offset, y_offset, 0.0f);
    drawTetrad(board->nextTetrad(), false);
    glPopMatrix();

    return true;
}

void TetrisRenderer::drawTetrad(const Tetrad & tetrad, bool dbg) {
    if (!tetrad.initialized()) {
        return;
    }

    if (dbg) {
        glPushMatrix();
        glLoadIdentity();
        glColor3f(1.0f, 1.0f, 1.0f);

        std::string txt;
        boost::shared_ptr<v3d::font::Font2D> font = fonts_->get("debug");
        v3d::gl::GLFontRenderer fontRenderer(*font, logger_);

        txt = "Offset x: " + tetrad.offset(Tetrad::OFFSET_X);
        fontRenderer.print(txt, 500.0f, 250.0f);

        txt = "width: " + tetrad.width();
        fontRenderer.print(txt, 500.0f, 275.0f);

        txt = "Offset y: " + tetrad.offset(Tetrad::OFFSET_Y);
        fontRenderer.print(txt, 500.0f, 300.0f);

        txt = "height: " + tetrad.height();
        fontRenderer.print(txt, 500.0f, 325.0f);

        txt = "pos x: " + tetrad.position().first;
        fontRenderer.print(txt, 500.0f, 350.0f);

        txt = "pos y: ", tetrad.position().second;
        fontRenderer.print(txt, 500.0f, 400.0f);

        glPopMatrix();
    }
    Tetrad::ShapeInfo shape = tetrad.shape();

    // set texture
    textures_[shape.color_]->bind();

    // render quads according to the layout bitmask
    float quad_size = 25.0f;
    float x_offset2 = tetrad.position().first * quad_size;
    float y_offset2 = tetrad.position().second * quad_size;
    float x_offset = 0.0f;
    float y_offset = 0.0f;
    glPushMatrix();
    glTranslatef(x_offset2, y_offset2, 0.0f);

    for (unsigned int i = 0; i < 4; i++) {
        for (unsigned int j = 0; j < 4; j++) {
            bool draw_empty = false;  // true;

            if (shape.layout_[i][j] > 0 || draw_empty) {
                glPushMatrix();
                glTranslatef(x_offset, y_offset, 0.0f);
                glScalef(quad_size, quad_size, quad_size);

                if (shape.layout_[i][j] == 0) {
                    textures_["cyan"]->bind();
                } else {
                    textures_[shape.color_]->bind();
                }

                // draw quad
                glBegin(GL_QUADS);

                glTexCoord2f(0.0f, 0.0f);
                glVertex3f(0.0f, 0.0f,  0.0f);

                glTexCoord2f(1.0f, 0.0f);
                glVertex3f(1.0f, 0.0f,  0.0f);

                glTexCoord2f(1.0f, 1.0f);
                glVertex3f(1.0f,  1.0f,  0.0f);

                glTexCoord2f(0.0f, 1.0f);
                glVertex3f(0.0f,  1.0f,  0.0f);

                glEnd();
                glPopMatrix();
            }
            x_offset += quad_size;
        }
        y_offset += quad_size;
        x_offset = 0.0f;
    }
    glPopMatrix();
}

void TetrisRenderer::drawPiece(const Piece & piece) {
    if (piece.color() == Piece::COLOR_EMPTY) {
        return;
    }

    // set texture
    textures_[piece.str()]->bind();
    // glColor3f(1.0f, 1.0f, 1.0f);

    // draw quad
    glBegin(GL_QUADS);

    glTexCoord2f(0.0f, 0.0f);
    glVertex3f(0.0f, 0.0f,  0.0f);

    glTexCoord2f(1.0f, 0.0f);
    glVertex3f(1.0f, 0.0f,  0.0f);

    glTexCoord2f(1.0f, 1.0f);
    glVertex3f(1.0f,  1.0f,  0.0f);

    glTexCoord2f(0.0f, 1.0f);
    glVertex3f(0.0f,  1.0f,  0.0f);

    glEnd();
}
