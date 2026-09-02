/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Renderer.h"

#include <cstddef>
#include <vector>

#include "../../api/render/realtime/Frame.h"
#include "../../api/render/realtime/Pass.h"

#include <boost/shared_ptr.hpp>

namespace v3d::editor {

    namespace {

        /**
         * What every viewport clears to.
         **/
        const glm::vec4 background(0.16f, 0.17f, 0.19f, 1.0f);

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
    }

    /**
     **/
    void Renderer::draw() {
        boost::shared_ptr<v3d::render::realtime::Frame> frame = engine_.frame();
        if (!frame) {
            return;
        }

        boost::shared_ptr<v3d::render::realtime::vulkan::LineRenderer> lines = engine_.lines();

        for (std::size_t index = 0; index < views_.size(); index++) {
            const boost::shared_ptr<ViewPort>& view = views_[index];
            if (!view) {
                continue;
            }

            view->draw(&canvases_[index]);

            boost::shared_ptr<v3d::render::realtime::Pass> pass = frame->pass(view->name());
            pass->viewport(view->region());
            pass->clearColour(background_);
            // every view depth tests, so a wireframe is occluded by what is in front of it.
            // depth is cleared with colour, and each pass clears only its own region
            pass->depth(true);
            pass->camera(view->camera()->view(), view->camera()->projection());

            if (lines) {
                lines->submit(canvases_[index], pass.get());
            }
        }

        engine_.renderFrame();
    }

    /**
     **/
    void Renderer::shutdown() {
        engine_.shutdown();
    }

};  // namespace v3d::editor
