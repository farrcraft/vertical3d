/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#include "../../api/asset/TextureFont.h"
#include "../../api/asset/Type.h"
#include "../../api/render/realtime/Frame.h"
#include "../../api/render/realtime/Pass.h"

#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    namespace {

        /**
         * What every viewport clears to.
         **/
        const glm::vec4 background(0.16f, 0.17f, 0.19f, 1.0f);

        /**
         * What a view's handle pass is called, on the end of the view's own name.
         **/
        const char* const handleSuffix = " handles";

        /**
         * The one pass that is not a view's, drawn over all of them.
         **/
        const char* const uiPass = "ui";

        /**
         * The glyphs the ui ever draws - printable ascii. The atlas goes to the device once
         * at load, so every glyph has to be packed into it before then.
         **/
        const wchar_t* const charcodes =
            L" !\"#$%&'()*+,-./0123456789:;<=>?"
            L"@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_"
            L"`abcdefghijklmnopqrstuvwxyz{|}~";

        /**
         * The size the font is rasterized at. Nothing scales a glyph, so it is also the size
         * the ui is drawn at, and what the bar and its rows are sized from.
         **/
        const float fontSize = 15.0f;

    };  // namespace

    /**
     **/
    Renderer::Renderer(const boost::shared_ptr<v3d::render::realtime::Window>& window,
        const boost::shared_ptr<v3d::log::Logger>& logger,
        const boost::shared_ptr<v3d::asset::Manager>& assetManager, entt::registry* registry) :
        logger_(logger),
        engine_(logger, assetManager, registry),
        background_(background) {
        engine_.initialize(window);
        engine_.clearColour(background_);

        loadFont(assetManager);

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

        v3d::ui::ComponentRenderer::Style& style = uiRenderer_->style();
        style.lineHeight = fontSize * 1.5f;
        style.padding = fontSize * 1.4f;
        style.barHeight = fontSize * 1.8f;
        style.panelPadding = fontSize * 0.3f;
    }

    /**
     **/
    void Renderer::loadFont(const boost::shared_ptr<v3d::asset::Manager>& assetManager) {
        // a one channel atlas: the glyph's coverage becomes its alpha, which is what lets text
        // go through the quad shader
        fontCache_ = boost::make_shared<v3d::font::TextureFontCache>(512, 512, v3d::font::TextureTextBuffer::LCD_FILTERING_OFF, logger_);
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
        if (!font || !font->font()) {
            logger_->get()->error("the ui font could not be loaded, so the menus will have no labels");
            return;
        }

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
    void Renderer::drawText(const std::string& text, const glm::vec2& pen, const glm::vec4& colour) {
        if (text.empty() || !markup_.font_ || !text_) {
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
    Renderer::~Renderer() {
    }

    /**
     **/
    void Renderer::views(const std::vector<boost::shared_ptr<ViewPort>>& views) {
        views_ = views;
        canvases_.resize(views_.size());
        overlays_.resize(views_.size());
    }

    /**
     **/
    void Renderer::scene(const boost::shared_ptr<Scene>& scene) {
        scene_ = scene;
    }

    /**
     **/
    void Renderer::manipulator(const boost::shared_ptr<Manipulator>& manipulator) {
        manipulator_ = manipulator;
    }

    /**
     **/
    void Renderer::ui(const boost::shared_ptr<v3d::ui::Engine>& ui) {
        ui_ = ui;
    }

    /**
     **/
    glm::vec2 Renderer::insets() const {
        if (!ui_ || !uiRenderer_) {
            return glm::vec2(0.0f, 0.0f);
        }
        return uiRenderer_->insets(*ui_);
    }

    /**
     **/
    void Renderer::draw() {
        boost::shared_ptr<v3d::render::realtime::Frame> frame = engine_.frame();
        if (!frame) {
            return;
        }

        boost::shared_ptr<v3d::render::realtime::vulkan::LineRenderer> lines = engine_.lines();

        // a view with no scene still draws its grid, which is what an empty document looks
        // like rather than an error
        const Scene empty;
        const Scene& scene = scene_ ? *scene_ : empty;

        for (std::size_t index = 0; index < views_.size(); index++) {
            const boost::shared_ptr<ViewPort>& view = views_[index];
            if (!view) {
                continue;
            }

            view->draw(scene, manipulator_.get(), &canvases_[index], &overlays_[index]);

            boost::shared_ptr<v3d::render::realtime::Pass> pass = frame->pass(view->name());
            pass->viewport(view->region());
            pass->clearColour(background_);
            // every view depth tests, so a wireframe is occluded by what is in front of it.
            // depth is cleared with colour, and each pass clears only its own region
            pass->depth(true);
            pass->camera(view->camera()->view(), view->camera()->projection());

            // the handles go over the top of what the scene pass left, undepth tested, so
            // that a handle lying in the plane of the grid is not lost to it
            boost::shared_ptr<v3d::render::realtime::Pass> overlay = frame->pass(view->name() + handleSuffix);
            overlay->viewport(view->region());
            overlay->keepColour();
            overlay->depth(false);
            overlay->camera(view->camera()->view(), view->camera()->projection());

            if (lines) {
                lines->submit(canvases_[index], pass.get());
                if (!overlays_[index].empty()) {
                    lines->submit(overlays_[index], overlay.get());
                }
            }
        }

        drawUi(frame);

        engine_.renderFrame();
    }

    /**
     **/
    void Renderer::drawUi(const boost::shared_ptr<v3d::render::realtime::Frame>& frame) {
        if (!ui_ || !uiRenderer_) {
            return;
        }
        const int width = engine_.window()->width();
        const int height = engine_.window()->height();
        if (width <= 0 || height <= 0) {
            return;
        }
        if (canvas_.width() != static_cast<uint32_t>(width) || canvas_.height() != static_cast<uint32_t>(height)) {
            canvas_.resize(static_cast<uint32_t>(width), static_cast<uint32_t>(height));
        }

        canvas_.clear();
        uiRenderer_->draw(&canvas_, *ui_);
        if (canvas_.empty()) {
            return;
        }

        // over the whole window rather than a view, and over what every view left. A canvas
        // carries its own projection, so this pass needs no camera
        boost::shared_ptr<v3d::render::realtime::Pass> pass = frame->pass(uiPass);
        pass->keepColour();
        pass->depth(false);
        engine_.quads()->submit(canvas_, pass.get());
    }

    /**
     **/
    void Renderer::shutdown() {
        engine_.shutdown();
    }

};  // namespace v3d::editor
