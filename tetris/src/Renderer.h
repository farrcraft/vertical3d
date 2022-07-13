/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>

#include "Piece.h"
#include "Tetrad.h"

#include "../../v3dlibs/hookah/Window.h"
#include "../../v3dlibs/gl/GLTexture.h"
#include "../../v3dlibs/font/FontCache.h"

#include <boost/shared_ptr.hpp>

class TetrisScene;

class TetrisRenderer {
 public:
    explicit TetrisRenderer(boost::shared_ptr<TetrisScene> scene);

    void draw(Hookah::Window * window);
    void resize(int width, int height);

    bool drawBoard();
    void drawPiece(const Piece & piece);
    void drawTetrad(const Tetrad & tetrad, bool dbg);

 private:
    boost::shared_ptr<TetrisScene> scene_;
    std::map<std::string, boost::shared_ptr<v3d::gl::GLTexture> > textures_;
    boost::shared_ptr<v3d::font::FontCache> fonts_;
};
