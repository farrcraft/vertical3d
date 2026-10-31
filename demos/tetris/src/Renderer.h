/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/asset/Manager.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/WindowRenderer.h>
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
 * Everything tetris draws, built as one canvas of quads and handed to the window renderer.
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
     * The size the ui and the side panel are drawn at. Glyphs are distance fields, so the atlas
     * is scaled to this size rather than rasterized at it.
     **/
    static constexpr float fontSize = 22.0F;

    /**
     * The piece colours, which are also the texture file names and the names a shape in
     * pieces/shapes.txt gives itself.
     **/
    static constexpr const char* colours[] = { "red", "cyan", "blue", "green", "orange", "purple", "yellow" };

    /**
     * The size of the atlas the seven 64x64 block textures are packed into. Three fit across a
     * row, so seven need three rows. The atlas keeps a one texel gutter around each texture.
     **/
    static constexpr unsigned int atlasSize = 256;

    /**
     * How many cells of well width the panel beside it is given, for the preview and the
     * score. The two together decide how big a cell can be.
     **/
    static constexpr float panelCells = 6.0F;

    static constexpr float margin = 16.0F;

    static constexpr glm::vec4 backgroundColour{0.09F, 0.09F, 0.11F, 1.0F};
    static constexpr glm::vec4 wellColour{0.04F, 0.04F, 0.05F, 1.0F};
    static constexpr glm::vec4 white{1.0F, 1.0F, 1.0F, 1.0F};

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

    v3d::render::realtime::WindowRenderer engine_;

    v3d::render::realtime::TextureHandle pieces_;
    std::map<std::string, Sprite> sprites_;

    // the shell's screen, built over engine_
    v3d::ui::shell::Screen* screen_ = nullptr;
};
