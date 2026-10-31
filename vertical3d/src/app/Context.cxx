/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Context.h"

#include <api/event/Event.h>
#include <api/type/camera/Camera.h>
#include <api/type/camera/Profile.h>
#include <vertical3d/src/document/Document.h>
#include <vertical3d/src/render/Shot.h>
#include <vertical3d/src/selection/SelectMask.h>
#include <vertical3d/src/view/ViewPort.h>

#include <cstddef>
#include <functional>
#include <string>
#include <variant>

#include <boost/filesystem/operations.hpp>
#include <boost/optional/optional.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::app {

/**
 **/
void Context::report(const document::Outcome& outcome) const {
    if (!outcome.message.empty()) {
        logger->get()->info("{}", outcome.message);
        if (notices) {
            notices->post(outcome.message);
        }
    }
}

/**
 **/
void Context::refuse(const std::string& message) const {
    report({ .done = false, .message = message });
}

/**
 **/
boost::shared_ptr<view::ViewPort> Context::activeView() const {
    return pointer ? pointer->activeView() : nullptr;
}

/**
 **/
std::size_t Context::areaOf(const v3d::event::Event& event) const {
    const boost::optional<v3d::event::EventData> data = event.data();
    if (data && std::holds_alternative<int>(*data)) {
        return layout->find(static_cast<std::size_t>(std::get<int>(*data)));
    }
    return pointer ? pointer->activeArea() : layout->areas().size();
}

/**
 **/
void Context::atView(const std::string& what, const std::function<void(const v3d::type::camera::Profile&)>& run) const {
    const boost::shared_ptr<view::ViewPort> view = activeView();
    if (!view || !view->camera()) {
        refuse(what + " at a view, and there is no active view");
        return;
    }
    run(view->camera()->profile());
}

/**
 **/
void Context::masked(selection::SelectMask mask, const std::string& name, const std::function<document::Outcome()>& operation) const {
    if (selectTool->mask() != mask) {
        refuse(name + " needs the " + selectMaskName(mask) + " select mask");
        return;
    }
    report(operation());
}

/**
 **/
v3d::type::camera::Profile Context::shot() const {
    return render::shotFrom(*document->scene(), pointer->activeView()->camera()->profile());
}

/**
 **/
std::string Context::projectKey() const {
    if (!document) {
        return {};
    }
    return boost::filesystem::absolute(document->project()->directory()).lexically_normal().generic_string();
}

};  // namespace v3d::editor::app
