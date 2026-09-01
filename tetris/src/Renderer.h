/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <map>
#include <string>

#include "Piece.h"
#include "Tetrad.h"
#include "TetrisScene.h"

#include "../../api/gl/GLTexture.h"
#include "../../api/render/realtime/Engine3D.h"

#include <boost/shared_ptr.hpp>

class TetrisScene;

class TetrisRenderer {
 public:
     TetrisRenderer(const boost::shared_ptr<v3d::render::realtime::Window3D>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
         const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry);

    void draw();
    void resize(int width, int height);

    bool drawBoard();
    void drawPiece(const Piece & piece);
    void drawTetrad(const Tetrad & tetrad, bool dbg);

    void scene(const boost::shared_ptr<TetrisScene>& scene);

 private:
    boost::shared_ptr<TetrisScene> scene_;
    std::map<std::string, boost::shared_ptr<v3d::gl::GLTexture> > textures_;
    boost::shared_ptr<v3d::log::Logger> logger_;
    v3d::render::realtime::Engine3D engine_;
};
