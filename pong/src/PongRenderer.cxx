/**
 * Vertical3D
 * Copyright(c) 2023 Joshua Farr(josh@farrcraft.com)
 **/

#include "PongRenderer.h"

#include <string>

#include "../../api/asset/TextureFont.h"

#include <boost/bind/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/make_shared.hpp>

namespace {

    /**
     * The glyphs pong ever draws - printable ascii. The atlas is uploaded to the device once
     * at load, so every glyph has to be packed into it before then.
     **/
    const wchar_t* const charcodes =
        L" !\"#$%&'()*+,-./0123456789:;<=>?"
        L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
        L"`abcdefghijklmnopqrstuvwxyz{|}~";

    /**
     * The size the font is rasterized at. Nothing scales a glyph, so this is also the size
     * everything is drawn at.
     **/
    const float fontSize = 28.0f;

    const glm::vec4 boardColour(0.35f, 0.35f, 0.35f, 1.0f);
    const glm::vec4 ballColour(1.0f, 1.0f, 1.0f, 1.0f);
    const glm::vec4 scoreColour(0.85f, 0.85f, 0.85f, 1.0f);

    const unsigned int ballSides = 32;
    const unsigned int wallThickness = 15;
    const unsigned int centreLineWidth = 14;

};  // namespace

/**
 **/
PongRenderer::PongRenderer(const boost::shared_ptr<v3d::render::realtime::Window>& window, const boost::shared_ptr<v3d::log::Logger>& logger,
    const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
    engine_(logger, assetManager, registry) {
    engine_.initialize(window);

    loadFont(assetManager, logger);

    uiRenderer_ = boost::make_shared<v3d::ui::ComponentRenderer>(
        [this](const std::string& text) -> float {
            float width = 0.0f;
            for (char character : text) {
                boost::shared_ptr<v3d::font::TextureFont::Glyph> glyph = markup_.font_->glyph(static_cast<wchar_t>(character));
                if (glyph) {
                    width += glyph->advance_.x;
                }
            }
            return width;
        },
        [this](const std::string& text, const glm::vec2& pen, const glm::vec4& colour) {
            drawText(text, pen, colour);
        });
    uiRenderer_->style().lineHeight = fontSize * 1.4f;
}

/**
 **/
void PongRenderer::loadFont(const boost::shared_ptr<v3d::asset::Manager>& assetManager, const boost::shared_ptr<v3d::log::Logger>& logger) {
    // a one channel atlas: the glyph's coverage becomes its alpha, which is what lets text
    // go through the quad shader. Subpixel (LCD) filtering would need dual source blending or
    // a second pass
    fontCache_ = boost::make_shared<v3d::font::TextureFontCache>(512, 512, v3d::font::TextureTextBuffer::LCD_FILTERING_OFF, logger);
    fontCache_->charcodes(charcodes);

    markup_.family_ = "sans";
    markup_.bold_ = false;
    markup_.italic_ = false;
    markup_.rise_ = 0.0f;
    markup_.spacing_ = 0.0f;
    markup_.gamma_ = 1.0f;
    markup_.outline_ = false;
    markup_.underline_ = false;
    markup_.overline_ = false;
    markup_.strikethrough_ = false;
    markup_.foregroundColor_ = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
    // transparent, so no background quad is emitted behind each glyph
    markup_.backgroundColor_ = glm::vec4(0.0f, 0.0f, 0.0f, 0.0f);
    markup_.size_ = fontSize;

    boost::shared_ptr<v3d::asset::Loader> loader = assetManager->resolveLoader(v3d::asset::Type::TextureFont);
    v3d::asset::ParameterValue value = markup_.size_;
    loader->parameter("fontSize", value);
    boost::shared_ptr<v3d::asset::TextureFont> font = boost::dynamic_pointer_cast<v3d::asset::TextureFont>(
        assetManager->load("fonts/NotoSans-Regular.ttf", v3d::asset::Type::TextureFont));

    font->font()->atlas(fontCache_->atlas());
    font->font()->loadGlyphs(charcodes);
    fontCache_->add(font->font());
    markup_.font_ = font->font();

    // every glyph is packed by now, so the atlas can go to the device once and stay there
    atlas_ = engine_.quads()->texture(fontCache_->atlas()->image());

    text_ = boost::make_shared<v3d::font::TextureTextBuffer>();
}

/**
 **/
void PongRenderer::scene(const boost::shared_ptr<PongScene>& scene) {
    scene_ = scene;
}

/**
 **/
void PongRenderer::ui(const boost::shared_ptr<v3d::ui::Engine>& ui) {
    ui_ = ui;
}

/**
 **/
void PongRenderer::resize(int width, int height) {
    if (scene_) {
        scene_->resize(width, height);
    }
    canvas_.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
}

/**
 **/
void PongRenderer::drawText(const std::string& text, const glm::vec2& pen, const glm::vec4& colour) {
    if (text.empty() || !markup_.font_) {
        return;
    }
    text_->clear();
    markup_.foregroundColor_ = colour;

    glm::vec2 cursor = pen;
    const std::wstring wide(text.begin(), text.end());
    text_->addText(&cursor, markup_, wide);

    canvas_.text(*text_, atlas_);
}

/**
 **/
void PongRenderer::draw() {
    if (!scene_) {
        return;
    }

    const int width = engine_.window()->width();
    const int height = engine_.window()->height();
    if (width <= 0 || height <= 0) {
        // a minimized window: the engine skips the frame, and a canvas with no area has no
        // projection to build geometry against
        engine_.renderFrame();
        return;
    }

    // no resize event reaches the renderer, so the window is the only thing that knows
    if (canvas_.width() != static_cast<uint32_t>(width) || canvas_.height() != static_cast<uint32_t>(height)) {
        resize(width, height);
    }

    canvas_.clear();

    drawBoard();
    drawPaddle(scene_->left());
    drawPaddle(scene_->right());
    drawBall();
    drawScores();

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
void PongRenderer::drawBoard() {
    const float width = static_cast<float>(engine_.window()->width());
    const float height = static_cast<float>(engine_.window()->height());
    const float half = centreLineWidth * 0.5f;
    const float wall = static_cast<float>(wallThickness);

    // centre line
    canvas_.rect(glm::vec2(width * 0.5f - half, 0.0f), glm::vec2(width * 0.5f + half, height), boardColour);
    // top and bottom walls
    canvas_.rect(glm::vec2(0.0f, 0.0f), glm::vec2(width, wall), boardColour);
    canvas_.rect(glm::vec2(0.0f, height - wall), glm::vec2(width, height), boardColour);
}

/**
 **/
void PongRenderer::drawScores() {
    const float width = static_cast<float>(engine_.window()->width());
    const float height = static_cast<float>(engine_.window()->height());

    const std::string left = boost::lexical_cast<std::string>(scene_->left().score());
    const std::string right = boost::lexical_cast<std::string>(scene_->right().score());

    drawText(left, glm::vec2(width * 0.25f, height * 0.25f), scoreColour);
    drawText(right, glm::vec2(width * 0.75f, height * 0.25f), scoreColour);
}

/**
 **/
void PongRenderer::drawBall() {
    canvas_.circle(scene_->ball().position(), scene_->ball().size(), ballSides, ballColour);
}

/**
 **/
void PongRenderer::drawPaddle(const Paddle& paddle) {
    canvas_.push();
    // the paddle's position is the centre of its travel; its rectangle is drawn from the corner
    canvas_.translate(glm::vec2(paddle.offset(), paddle.position() - 25.0f));
    const glm::vec3 colour = paddle.color();
    canvas_.rect(glm::vec2(0.0f, 0.0f), glm::vec2(15.0f, 50.0f), glm::vec4(colour, 1.0f));
    canvas_.pop();
}
