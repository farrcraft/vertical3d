/**
 * Vertical3D
 * Copyright(c) 2022 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <api/asset/Image.h>
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
 * The size the ui and the side panel are drawn at, which the one atlas is scaled to per
 * ADR-0036 rather than rasterized at.
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

constexpr glm::vec4 wellColour(0.04f, 0.04f, 0.05f, 1.0f);
constexpr glm::vec4 borderColour(0.30f, 0.30f, 0.34f, 1.0f);
constexpr glm::vec4 textColour(0.85f, 0.85f, 0.85f, 1.0f);
constexpr glm::vec4 white(1.0f, 1.0f, 1.0f, 1.0f);

};  // namespace

/**
 **/
TetrisRenderer::TetrisRenderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    logger_(logger), engine_(logger, assetManager, registry) {
    engine_.initialize(window);
    engine_.clearColour(glm::vec4(0.09f, 0.09f, 0.11f, 1.0f));

    loadPieces(assetManager, logger);
    const boost::shared_ptr<v3d::render::realtime::vulkan::renderer::Quad> quads = engine_.quads();
    text_ = boost::make_shared<v3d::ui::paint::TextRenderer>(assetManager, logger,
        [quads](const boost::shared_ptr<v3d::image::Image>& atlas) {
            return quads->texture(atlas);
        });

    uiRenderer_ = boost::make_shared<v3d::ui::paint::ComponentRenderer>(text_->measure(fontSize), text_->write(&canvas_, fontSize));
    uiRenderer_->dressing().lineHeight = fontSize * 1.4f;
}

/**
 **/
void TetrisRenderer::loadPieces(const boost::shared_ptr<v3d::asset::Manager>& assetManager, const boost::shared_ptr<v3d::log::Logger>& logger) {
    // one atlas rather than seven textures: a block is the only thing the well is made of,
    // so packing them means the whole board is one batch instead of a flush per colour
    v3d::image::TextureAtlas atlas(atlasSize, atlasSize, 3, logger);

    for (const char* const colour : colours) {
        const std::string name = std::string("pieces/") + colour + ".tga";
        boost::shared_ptr<v3d::asset::Image> asset =
            boost::dynamic_pointer_cast<v3d::asset::Image>(assetManager->load(name, v3d::asset::Type::ImageTga));
        if (!asset || !asset->image()) {
            logger_->get()->error("unable to load the piece texture {}", name);
            continue;
        }
        boost::shared_ptr<v3d::image::Image> image = asset->image();
        if (image->format() != v3d::image::Image::Format::RGB) {
            logger_->get()->error("the piece texture {} is not 24 bit, which is what the atlas packs", name);
            continue;
        }

        const glm::ivec4 region = atlas.region(image->width(), image->height());
        if (region.x < 0) {
            logger_->get()->error("the piece atlas is too small to fit {}", name);
            continue;
        }
        atlas.region(region.x, region.y, region.z, region.w, image->data(), image->width() * 3);

        // half a texel in on every side, so that filtering a block down to a cell cannot
        // reach across the packer's border into its neighbour
        const float size = static_cast<float>(atlasSize);
        Sprite sprite;
        sprite.uv0 = glm::vec2((region.x + 0.5f) / size, (region.y + 0.5f) / size);
        sprite.uv1 = glm::vec2((region.x + region.z - 0.5f) / size, (region.y + region.w - 0.5f) / size);
        sprites_[colour] = sprite;
    }

    pieces_ = engine_.quads()->texture(atlas.image());
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
    canvas_.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

/**
 **/
TetrisRenderer::Layout TetrisRenderer::layout() const {
    const GameBoard* board = scene_->board();
    const float rows = static_cast<float>(board->rows());
    const float columns = static_cast<float>(board->columns());
    const float width = static_cast<float>(canvas_.width());
    const float height = static_cast<float>(canvas_.height());

    Layout metrics;
    // the well has to fit vertically, and the well plus its panel horizontally
    metrics.cell = std::max(1.0f, std::min((height - 2.0f * margin) / rows, (width - 2.0f * margin) / (columns + panelCells)));
    const float total = metrics.cell * (columns + panelCells);
    metrics.origin = glm::vec2((width - total) * 0.5f, (height - metrics.cell * rows) * 0.5f);
    return metrics;
}

/**
 **/
void TetrisRenderer::draw() {
    if (!scene_) {
        return;
    }

    glm::ivec2 size;
    if (!engine_.beginFrame(&size)) {
        return;
    }
    if (canvas_.width() != static_cast<uint32_t>(size.x) || canvas_.height() != static_cast<uint32_t>(size.y)) {
        resize(size.x, size.y);
    }

    canvas_.clear();

    const Layout metrics = layout();
    drawWell(metrics);
    drawBlocks(metrics);
    drawFalling(metrics);
    drawPanel(metrics);

    if (ui_) {
        uiRenderer_->draw(&canvas_, *ui_);
    }

    boost::shared_ptr<v3d::render::realtime::Pass> pass =
        engine_.frame()->pass(v3d::render::realtime::Engine3D::colourPass);
    engine_.quads()->submit(canvas_, pass.get());

    engine_.renderFrame();
}

/**
 **/
void TetrisRenderer::drawWell(const Layout& metrics) {
    const GameBoard* board = scene_->board();
    const glm::vec2 min = metrics.origin;
    const glm::vec2 max = min + glm::vec2(metrics.cell * board->columns(), metrics.cell * board->rows());
    const float border = 2.0f;

    canvas_.rect(min - glm::vec2(border, border), max + glm::vec2(border, border), borderColour);
    canvas_.rect(min, max, wellColour);
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
            const glm::vec2 min = metrics.origin + glm::vec2(column * metrics.cell, row * metrics.cell);
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
    const glm::vec2 origin = metrics.origin + glm::vec2(position.first * metrics.cell, position.second * metrics.cell);
    drawTetrad(tetrad, origin, metrics.cell);
}

/**
 **/
void TetrisRenderer::drawTetrad(const Tetrad& tetrad, const glm::vec2& origin, float cell) {
    if (!tetrad.initialized()) {
        return;
    }
    const Tetrad::ShapeInfo& shape = tetrad.shape();
    // the layout's first index is the row and its second the column, which is the order the
    // board's collision and lock-in walk it in
    for (unsigned int row = 0; row < 4; row++) {
        for (unsigned int column = 0; column < 4; column++) {
            if (shape.layout_[row][column] == 0) {
                continue;
            }
            drawBlock(shape.color_, origin + glm::vec2(column * cell, row * cell), cell);
        }
    }
}

/**
 **/
void TetrisRenderer::drawBlock(const std::string& colour, const glm::vec2& min, float cell) {
    std::map<std::string, Sprite>::const_iterator sprite = sprites_.find(colour);
    if (sprite == sprites_.end()) {
        return;
    }
    canvas_.rect(min, min + glm::vec2(cell, cell), sprite->second.uv0, sprite->second.uv1, white, pieces_);
}

/**
 **/
void TetrisRenderer::drawPanel(const Layout& metrics) {
    const GameBoard* board = scene_->board();
    const float line = fontSize * 1.4f;
    const glm::vec2 panel = metrics.origin + glm::vec2(metrics.cell * (board->columns() + 1.0f), line);

    text_->draw(&canvas_, "SCORE", panel, textColour, fontSize);
    text_->draw(&canvas_, boost::lexical_cast<std::string>(scene_->score()), panel + glm::vec2(0.0f, line), textColour, fontSize);

    text_->draw(&canvas_, "NEXT", panel + glm::vec2(0.0f, line * 3.0f), textColour, fontSize);
    // the preview is drawn a little smaller than the well, so a four wide tetrad fits the
    // panel it was given
    drawTetrad(board->nextTetrad(), panel + glm::vec2(0.0f, line * 3.5f), metrics.cell * 0.75f);

    if (board->over()) {
        text_->draw(&canvas_, "GAME OVER", panel + glm::vec2(0.0f, line * 7.0f), textColour, fontSize);
    }

    if (scene_->debug()) {
        const Tetrad current = board->currentTetrad();
        const Tetrad::PositionType position = current.position();
        const std::string state =
            boost::lexical_cast<std::string>(position.first) + "," + boost::lexical_cast<std::string>(position.second) +
            " " + boost::lexical_cast<std::string>(current.width()) + "x" + boost::lexical_cast<std::string>(current.height());
        text_->draw(&canvas_, state, panel + glm::vec2(0.0f, line * 9.0f), textColour, fontSize);
    }
}
