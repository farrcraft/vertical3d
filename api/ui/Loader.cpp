/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Loader.h"

#include <api/event/Router.h>
#include <api/log/Logger.h>

#include <string>
#include <vector>

#include <boost/json/object.hpp>
#include <entt/entt.hpp>

namespace v3d::ui {

Loader::Loader(const boost::shared_ptr<v3d::event::Router>& router,
    const boost::shared_ptr<entt::dispatcher>& dispatcher,
    const boost::shared_ptr<v3d::log::Logger>& logger) :
    themes_(logger),
    components_(router, dispatcher, logger) {
}

bool Loader::load(const boost::json::object& doc) {
    return themes_.read(doc) && components_.read(doc);
}

std::vector<boost::shared_ptr<Container>>& Loader::containers() noexcept {
    return components_.containers();
}

std::vector<boost::shared_ptr<style::Theme>>& Loader::themes() noexcept {
    return themes_.themes();
}

const std::string& Loader::active() const noexcept {
    return themes_.active();
}

};  // namespace v3d::ui
