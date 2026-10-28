/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/ui/loader/ComponentReader.h>
#include <api/ui/loader/ThemeReader.h>

#include <string>
#include <vector>

#include <boost/json/object.hpp>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>

namespace v3d::ui {

class Container;

namespace style {
class Theme;
};  // namespace style

/**
 * Builds containers and themes out of a ui document.
 *
 * Separate from Interface, which holds the built ui. A loader runs once, and a ui is queried
 * for as long as the app lives. Keeping them apart keeps the parsing out of Interface.h,
 * which every app that draws a ui compiles.
 *
 * The themes are read by loader::ThemeReader and the containers by loader::ComponentReader.
 * Both hold a document to the one rule loader/Members.h states. Nothing here is kept: load()
 * fills the two collections, and Interface takes them.
 **/
class Loader final {
 public:
    /**
     * @param router where a command's context is looked up
     * @param dispatcher what a built menu or toolbar sends its events to
     * @param logger where a document that does not read is reported
     **/
    Loader(const boost::shared_ptr<v3d::event::Router>& router,
        const boost::shared_ptr<entt::dispatcher>& dispatcher,
        const boost::shared_ptr<v3d::log::Logger>& logger);

    /**
     * Read the themes and the containers a document names.
     * @return false when the document is refused, leaving what was read incomplete
     **/
    bool load(const boost::json::object& doc);

    /**
     * @return the containers, in the order the document listed them
     **/
    std::vector<boost::shared_ptr<Container>>& containers() noexcept;

    /**
     * @return the themes, in the order the document listed them
     **/
    std::vector<boost::shared_ptr<style::Theme>>& themes() noexcept;

    /**
     * @return the name of the theme the document makes active, or empty when it names
     *      none. Interface then makes the first theme loaded active
     **/
    const std::string& active() const noexcept;

 private:
    loader::ThemeReader themes_;
    loader::ComponentReader components_;
};

};  // namespace v3d::ui
