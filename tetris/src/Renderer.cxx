/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/media/kind/Image.h>
#include <api/asset/Type.h>
#include <api/image/Image.h>
#include <api/image/TextureAtlas.h>

#include <algorithm>
#include <map>
#include <string>
#include <vector>

#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

namespace {

/**
 * The size the ui and the side panel are drawn at. Glyphs are distance fields, so the atlas
 * is scaled to this size rather than rasterized at it.
 **/
const float fontSize = 22.0f;

/**
 * The piece colours, which are also the texture file names and the names a shape in
 * pieces/shapes.txt gives itself.
 **/
const char* const colours[] = { "red", "cyan", "blue", "green", "orange", "purple", "yellow" };

/**
 * The atlas the seven 64x64 block textures are packed into. Three fit across a row, so
 * seven need three rows, and the packer keeps a one pixel border on every side.
 **/
const unsigned int atlasSize = 256;

/**
 * How many cells of well width the panel beside it is given, for the preview and the
 * score. The two together decide how big a cell can be.
 **/
const float panelCells = 6.0f;

const float margin = 16.0f;

/**
 * The height of a line of text, as a multiple of the text's size.
 **/
constexpr float lineSpacing = 1.4f;

constexpr glm::vec4 backgroundColour(0.09f, 0.09f, 0.11f, 1.0f);
constexpr glm::vec4 wellColour(0.04f, 0.04f, 0.05f, 1.0f);
constexpr glm::vec4 borderColour(0.30f, 0.30f, 0.34f, 1.0f);
constexpr glm::vec4 textColour(0.85f, 0.85f, 0.85f, 1.0f);
constexpr glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);

};  // namespace

/**
 **/
TetrisRenderer::TetrisRenderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager) :
    logger_(logger), engine_(logger, assetManager) {
    engine_.initialize(window);
    engine_.clearColour(backgroundColour);

    loadPieces(assetManager, logger);

    v3d::ui::shell::Screen::Options options;
    options.size = fontSize;
    screen_ = boost::make_shared<v3d::ui::shell::Screen>(&engine_, assetManager, logger, options);
}

/**
 **/
const boost::shared_ptr<v3d::ui::shell::StatisticsOverlay>& TetrisRenderer::statistics() const {
    return screen_->statistics();
}

/**
 **/
void TetrisRenderer::loadPieces(const boost::shared_ptr<v3d::asset::Manager>& assetManager, const boost::shared_ptr<v3d::log::Logger>& logger) {
    // one atlas rather than seven textures: a block is the only thing the well is made of,
    // so packing them means the whole board is one batch instead of a flush per colour
    v3d::image::TextureAtlas atlas(atlasSize, atlasSize, 3, logger);

    for (const char* const colour : colours) {
        const std::string name = std::string("pieces/") + colour + ".tga";
        const boost::shared_ptr<v3d::asset::media::kind::Image> asset =
            assetManager->load<v3d::asset::media::kind::Image>(name, v3d::asset::Type::ImageTga);
        if (!asset || !asset->image()) {
            logger_->get()->error("unable to load the piece texture {}", name);
            continue;
        }
        const boost::shared_ptr<v3d::image::Image> image = asset->image();
        if (image->format() != v3d::image::Image::Format::RGB) {
            logger_->get()->error("the piece texture {} is not 24 bit, which is what the atlas packs", name);
            continue;
        }

        const glm::ivec4 region = atlas.region(image->width(), image->height());
        if (region.x < 0) {
            logger_->get()->error("the piece atlas is too small to fit {}", name);
            continue;
        }
        atlas.region(region, image->data(), image->width() * 3);

        // half a texel in on every side, so that filtering a block down to a cell samples
        // texel centres rather than the empty gutter the atlas packs around a region
        const float size = static_cast<float>(atlasSize);
        Sprite sprite;
        sprite.uv0 = glm::vec2((static_cast<float>(region.x) + 0.5f) / size, (static_cast<float>(region.y) + 0.5f) / size);
        sprite.uv1 = glm::vec2((static_cast<float>(region.x + region.z) - 0.5f) / size, (static_cast<float>(region.y + region.w) - 0.5f) / size);
        sprites_[colour] = sprite;
    }

    pieces_ = engine_.textures()->texture(atlas.image());
}

/**
 **/
void TetrisRenderer::scene(const boost::shared_ptr<TetrisScene>& scene) {
    scene_ = scene;
}

/**
 **/
void TetrisRenderer::ui(const boost::shared_ptr<v3d::ui::Engine>& ui) {
    ui_ = ui;
}

/**
 **/
void TetrisRenderer::shutdown() {
    engine_.shutdown();
}

/**
 **/
void TetrisRenderer::resize(int width, int height) {
    if (scene_) {
        scene_->resize(width, height);
    }
}

/**
 **/
TetrisRenderer::Layout TetrisRenderer::layout() const {
    const GameBoard* board = scene_->board();
    const float rows = static_cast<float>(board->rows());
    const float columns = static_cast<float>(board->columns());
    const float width = static_cast<float>(screen_->canvas().width());
    const float height = static_cast<float>(screen_->canvas().height());

    Layout metrics;
    // the well has to fit vertically, and the well plus its panel horizontally
    metrics.cell = std::max(1.0f, std::min((height - (2.0f * margin)) / rows, (width - (2.0f * margin)) / (columns + panelCells)));
    const float total = metrics.cell * (columns + panelCells);
    metrics.origin = glm::vec2((width - total) * 0.5f, (height - (metrics.cell * rows)) * 0.5f);
    return metrics;
}

/**
 **/
void TetrisRenderer::draw(const v3d::ui::shell::StatisticsOverlay::Sample& statistics) {
    if (!scene_) {
        return;
    }

    if (!screen_->begin()) {
        return;
    }
    if (screen_->resized()) {
        resize(static_cast<int>(screen_->canvas().width()), static_cast<int>(screen_->canvas().height()));
    }

    const Layout metrics = layout();
    drawWell(metrics);
    drawBlocks(metrics);
    drawFalling(metrics);
    drawPanel(metrics);

    screen_->draw(ui_.get(), statistics);

    const boost::shared_ptr<v3d::render::realtime::Pass> pass =
        engine_.frame()->pass(v3d::render::realtime::Engine3D::colourPass);
    engine_.quads()->submit(screen_->canvas(), pass.get());

    engine_.renderFrame();
}

/**
 **/
void TetrisRenderer::drawWell(const Layout& metrics) {
    const GameBoard* board = scene_->board();
    const glm::vec2 min = metrics.origin;
    const glm::vec2 max = min + glm::vec2(metrics.cell * static_cast<float>(board->columns()), metrics.cell * static_cast<float>(board->rows()));
    const float border = 2.0f;

    screen_->canvas().rect(min - glm::vec2(border, border), max + glm::vec2(border, border), borderColour);
    screen_->canvas().rect(min, max, wellColour);
}

/**
 **/
void TetrisRenderer::drawBlocks(const Layout& metrics) {
    const GameBoard* board = scene_->board();
    for (unsigned int row = 0; row < board->rows(); row++) {
        for (unsigned int column = 0; column < board->columns(); column++) {
            const Piece piece = board->piece(column, row);
            if (piece.color() == Piece::COLOR_EMPTY) {
                continue;
            }
            const glm::vec2 min = metrics.origin + glm::vec2(static_cast<float>(column) * metrics.cell, static_cast<float>(row) * metrics.cell);
            drawBlock(piece.str(), min, metrics.cell);
        }
    }
}

/**
 **/
void TetrisRenderer::drawFalling(const Layout& metrics) {
    const Tetrad& tetrad = scene_->board()->currentTetrad();
    const Tetrad::PositionType position = tetrad.position();
    // a tetrad's position is the cell its 4x4 layout starts at, not the cell its first
    // filled block is in
    const glm::vec2 origin = metrics.origin + glm::vec2(static_cast<float>(position.first) * metrics.cell, static_cast<float>(position.second) * metrics.cell);
    drawTetrad(tetrad, origin, metrics.cell);
}

/**
 **/
void TetrisRenderer::drawTetrad(const Tetrad& tetrad, const glm::vec2& origin, float cell) {
    if (!tetrad.initialized()) {
        return;
    }
    const Tetrad::ShapeInfo& shape = tetrad.shape();
    // the layout's first index is the row and its second the column, the same order the
    // board's collision and lock-in code reads it in
    for (unsigned int row = 0; row < 4; row++) {
        for (unsigned int column = 0; column < 4; column++) {
            if (shape.layout_[row][column] == 0) {
                continue;
            }
            drawBlock(shape.color_, origin + glm::vec2(static_cast<float>(column) * cell, static_cast<float>(row) * cell), cell);
        }
    }
}

/**
 **/
void TetrisRenderer::drawBlock(const std::string& colour, const glm::vec2& min, float cell) {
    const std::map<std::string, Sprite>::const_iterator sprite = sprites_.find(colour);
    if (sprite == sprites_.end()) {
        return;
    }
    screen_->canvas().rect(min, min + glm::vec2(cell, cell), sprite->second.uv0, sprite->second.uv1, white, pieces_);
}

/**
 **/
void TetrisRenderer::drawPanel(const Layout& metrics) {
    const GameBoard* board = scene_->board();
    const float line = fontSize * lineSpacing;
    // the panel's rows, counted in lines from its top
    constexpr float nextLabelRow = 3.0f;
    constexpr float previewRow = 3.5f;
    constexpr float gameOverRow = 7.0f;
    constexpr float debugRow = 9.0f;
    // the preview is drawn a little smaller than the well, so a four wide tetrad fits the
    // panel it was given
    constexpr float previewScale = 0.75f;
    const glm::vec2 panel = metrics.origin + glm::vec2(metrics.cell * (static_cast<float>(board->columns()) + 1.0f), line);

    screen_->text()->draw(&screen_->canvas(), "SCORE", panel, textColour, fontSize);
    screen_->text()->draw(&screen_->canvas(), boost::lexical_cast<std::string>(scene_->score()), panel + glm::vec2(0.0f, line), textColour, fontSize);

    screen_->text()->draw(&screen_->canvas(), "NEXT", panel + glm::vec2(0.0f, line * nextLabelRow), textColour, fontSize);
    drawTetrad(board->nextTetrad(), panel + glm::vec2(0.0f, line * previewRow), metrics.cell * previewScale);

    if (board->over()) {
        screen_->text()->draw(&screen_->canvas(), "GAME OVER", panel + glm::vec2(0.0f, line * gameOverRow), textColour, fontSize);
    }

    if (scene_->debug()) {
        const Tetrad current = board->currentTetrad();
        const Tetrad::PositionType position = current.position();
        const std::string state =
            boost::lexical_cast<std::string>(position.first) + "," + boost::lexical_cast<std::string>(position.second) +
            " " + boost::lexical_cast<std::string>(current.width()) + "x" + boost::lexical_cast<std::string>(current.height());
        screen_->text()->draw(&screen_->canvas(), state, panel + glm::vec2(0.0f, line * debugRow), textColour, fontSize);
    }
}
