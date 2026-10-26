/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/Engine3D.h>
#include <api/shell/Shell.h>
#include <api/ui/shell/Screen.h>

#include <map>
#include <string>

#include "Piece.h"
#include "Tetrad.h"
#include "TetrisScene.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

class TetrisScene;

/**
 * Everything tetris draws, built as one canvas of quads and handed to the render engine.
 *
 * Every block samples one atlas, so the well, the falling tetrad and the preview reach the
 * device as a single batch of quads. Text and the menu are quads too, with their own textures.
 **/
class TetrisRenderer final {
 public:
    /**
     * @param shell the app's shell, which builds its screen over this renderer's engine. It
     *        outlives the renderer
     **/
     TetrisRenderer(const boost::shared_ptr<v3d::window::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
         const boost::shared_ptr<v3d::asset::Manager>& assetManager, v3d::shell::Shell* shell);

    void draw();
    void resize(int width, int height);

    void scene(const boost::shared_ptr<TetrisScene>& scene);

    /**
     * Wait for everything in flight, before the window the device draws to goes away.
     **/
    void shutdown();

 private:
    /**
     * Where in the atlas one piece colour was packed.
     **/
    struct Sprite final {
        glm::vec2 uv0;
        glm::vec2 uv1;
    };

    /**
     * Where the well sits in the window, and how big one cell of it is. Recomputed every frame
     * from the canvas size, so the board stays centred and fills the window.
     **/
    struct Layout final {
        glm::vec2 origin;  /**< the top left of the well **/
        float cell;        /**< the side of one block **/
    };

    /**
     * Pack the seven piece textures into one atlas and upload it.
     **/
    void loadPieces(const boost::shared_ptr<v3d::asset::Manager>& assetManager, const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * @return where the well goes in a window of the canvas's current size
     **/
    Layout layout() const;

    void drawWell(const Layout& metrics);
    void drawBlocks(const Layout& metrics);
    void drawFalling(const Layout& metrics);
    void drawTetrad(const Tetrad& tetrad, const glm::vec2& origin, float cell);
    void drawPanel(const Layout& metrics);

    /**
     * One block of the atlas at a cell position.
     **/
    void drawBlock(const std::string& colour, const glm::vec2& min, float cell);

    boost::shared_ptr<TetrisScene> scene_;
    v3d::shell::Shell* shell_;
    boost::shared_ptr<v3d::log::Logger> logger_;

    v3d::render::realtime::Engine3D engine_;

    v3d::render::realtime::TextureHandle pieces_;
    std::map<std::string, Sprite> sprites_;

    // the shell's screen, built over engine_
    v3d::ui::shell::Screen* screen_ = nullptr;
};
