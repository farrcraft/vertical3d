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

#include "../../api/asset/Manager.h"
#include "../../api/render/realtime/Canvas.h"
#include "../../api/render/realtime/Engine3D.h"
#include "../../api/ui/ComponentRenderer.h"
#include "../../api/ui/Engine.h"
#include "../../api/ui/TextRenderer.h"

#include <boost/shared_ptr.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

class TetrisScene;

/**
 * Everything tetris draws, built as one canvas of quads and handed to the render engine.
 *
 * Every block on the board samples one atlas, so the whole well, the falling tetrad and the
 * preview reach the device as a single batch - see ADR-0005. Text and the menu are the same
 * primitive against their own textures.
 **/
class TetrisRenderer final {
 public:
     TetrisRenderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
         const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry);

    void draw();
    void resize(int width, int height);

    void scene(const boost::shared_ptr<TetrisScene>& scene);

    /**
     * The ui whose containers are drawn over the game, or null to draw none.
     **/
    void ui(const boost::shared_ptr<v3d::ui::Engine>& ui);

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
     * Where the well sits in the window, and how big one cell of it is. Recomputed on a
     * resize so the board stays centred and fills the window it is given.
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
    boost::shared_ptr<v3d::ui::Engine> ui_;
    boost::shared_ptr<v3d::log::Logger> logger_;

    v3d::render::realtime::Canvas canvas_;
    v3d::render::realtime::Engine3D engine_;

    v3d::render::realtime::TextureHandle pieces_;
    std::map<std::string, Sprite> sprites_;

    boost::shared_ptr<v3d::ui::TextRenderer> text_;
    boost::shared_ptr<v3d::ui::ComponentRenderer> uiRenderer_;
};
