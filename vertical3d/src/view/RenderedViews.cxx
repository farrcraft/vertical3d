/*
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 */

#include "RenderedViews.h"

#include <api/image/Image.h>
#include <api/render/realtime/canvas/Canvas.h>
#include <api/render/realtime/resource/Handle.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/render/Delegate.h>
#include <vertical3d/src/scene/RenderSettings.h>
#include <vertical3d/src/scene/Scene.h>

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <map>
#include <optional>
#include <set>
#include <string>
#include <vector>

#include "Shading.h"
#include "ViewPort.h"

#include <boost/shared_ptr.hpp>
#include <glm/common.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::editor::view {

/**
 **/
RenderedViews::RenderedViews(const Make& make, const Show& show) :
    make_(make),
    show_(show) {
}

/**
 **/
void RenderedViews::tick(const scene::Scene& scene, const std::vector<boost::shared_ptr<ViewPort>>& shown) {
    std::set<std::string> rendered;
    for (const boost::shared_ptr<ViewPort>& view : shown) {
        if (!view || view->display().shading != Shading::Rendered || !view->camera()) {
            continue;
        }
        rendered.insert(view->name());
        scene::RenderSettings settings = scene.renderSettings();
        settings.size = resolution(*view);
        const v3d::type::camera::Profile& from = view->camera()->profile();

        auto found = held_.find(view->name());
        // a view made again under the same name starts a session of its own
        if (found != held_.end() && found->second.view.lock() != view) {
            end(found);
            found = held_.end();
        }
        if (found == held_.end()) {
            const Held made{ .view = view, .delegate = make_() };
            made.delegate->render(scene, from, settings);
            found = held_.emplace(view->name(), made).first;
        } else {
            Held& held = found->second;
            // an edit starts a paused render again, and a view shown again with no edit samples on
            if (!held.delegate->update(scene, from, settings) && held.paused) {
                held.delegate->resume();
            }
            held.paused = false;
        }

        Held& held = found->second;
        const render::Delegate::Status status = held.delegate->status();
        if (status.pictures != held.taken) {
            held.taken = status.pictures;
            show_(found->first, held.delegate->picture(render::Delegate::Channel::Beauty));
        }
    }

    for (auto at = held_.begin(); at != held_.end();) {
        if (rendered.contains(at->first)) {
            ++at;
            continue;
        }
        const boost::shared_ptr<ViewPort> view = at->second.view.lock();
        if (!view || view->display().shading != Shading::Rendered) {
            at = end(at);
            continue;
        }
        if (!at->second.paused) {
            at->second.delegate->cancel();
            at->second.paused = true;
        }
        ++at;
    }
}

/**
 **/
void RenderedViews::release() {
    while (!held_.empty()) {
        end(held_.begin());
    }
}

/**
 **/
std::size_t RenderedViews::sessions() const noexcept {
    return held_.size();
}

/**
 **/
boost::shared_ptr<render::Delegate> RenderedViews::session(const std::string& view) const {
    const auto found = held_.find(view);
    return found == held_.end() ? boost::shared_ptr<render::Delegate>() : found->second.delegate;
}

/**
 **/
glm::uvec2 RenderedViews::resolution(const ViewPort& view) {
    const std::optional<glm::vec4>& frame = view.frame();
    const glm::vec2 area = frame ? glm::vec2(frame->z, frame->w) : glm::vec2(view.region().z, view.region().w);
    const float pixel = static_cast<float>(std::max(view.display().pixelSize, 1U));
    const auto side = [pixel](float length) {
        const float pixels = std::clamp(std::ceil(length / pixel), 1.0F, static_cast<float>(scene::RenderSettings::LARGEST_SIDE));
        return static_cast<unsigned int>(pixels);  // checked: clamped to [1, LARGEST_SIDE] first
    };
    return { side(area.x), side(area.y) };
}

/**
 **/
void RenderedViews::cover(const ViewPort& view, const v3d::render::realtime::resource::TextureHandle& picture, v3d::render::realtime::canvas::Canvas* canvas) {
    const glm::vec4& region = view.region();
    const glm::vec4 into = view.frame().value_or(glm::vec4(0.0F, 0.0F, region.z, region.w));
    canvas->resize(glm::uvec2(glm::max(glm::vec2(region.z, region.w), glm::vec2(1.0F))));
    canvas->clear();
    canvas->rect(glm::vec2(into.x, into.y), glm::vec2(into.x + into.z, into.y + into.w), glm::vec2(0.0F), glm::vec2(1.0F), glm::vec4(1.0F), picture);
}

/**
 **/
std::map<std::string, RenderedViews::Held>::iterator RenderedViews::end(const std::map<std::string, Held>::iterator& held) {
    const std::string name = held->first;
    // the delegate stops its render and waits for its thread as it goes
    held->second.delegate->cancel();
    const auto next = held_.erase(held);
    show_(name, boost::shared_ptr<v3d::image::Image>());
    return next;
}

};  // namespace v3d::editor::view
