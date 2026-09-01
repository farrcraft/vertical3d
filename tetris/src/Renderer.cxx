/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include <GL/glew.h>

#include <string>

#include "Renderer.h"
#include "TetrisScene.h"

#include "../../api/image/Factory.h"

#include <boost/bind.hpp>
#include <boost/make_shared.hpp>

TetrisRenderer::TetrisRenderer(const boost::shared_ptr<v3d::render::realtime::Window3D>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    logger_(logger), engine_(logger, assetManager, registry) {
    engine_.initialize(window);

    try {
        v3d::image::Factory factory(logger);
        // load textures
        boost::shared_ptr<v3d::gl::GLTexture> texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/red.tga"), logger);
        textures_["red"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/blue.tga"), logger);
        textures_["blue"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/cyan.tga"), logger);
        textures_["cyan"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/green.tga"), logger);
        textures_["green"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/orange.tga"), logger);
        textures_["orange"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/purple.tga"), logger);
        textures_["purple"] = texture;
        texture = boost::make_shared<v3d::gl::GLTexture>(factory.read("pieces/yellow.tga"), logger);
        textures_["yellow"] = texture;
    }
    catch (...) {
        logger_->get()->error("texture load exception!");
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

void TetrisRenderer::draw() {
    const int width = engine_.window()->width();
    const int height = engine_.window()->height();
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glLoadIdentity();
    glPushAttrib(GL_TEXTURE_BIT);
    // flood fill the entire window with dark grey
    glDisable(GL_TEXTURE_2D);
    // glBindTexture(GL_TEXTURE_2D, 0);
    glColor3f(0.35f, 0.35f, 0.35f);
    glBegin(GL_QUADS);
        glVertex2i(0, 0);
        glVertex2i(width, 0);
        glVertex2i(width, height);
        glVertex2i(0, height);
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

    // TODO(josh): debug text needs rewriting against operation::TextureFont - see
    // docs/plans/Modernization.md phase 4. The block that was here drew through a
    // FontCache and GLFontRenderer that no longer exist, and built its strings by
    // adding ints to string literals.

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

void TetrisRenderer::scene(const boost::shared_ptr<TetrisScene>& scene) {
    scene_ = scene;
}
