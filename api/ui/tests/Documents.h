/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/event/Router.h>
#include <api/log/Logger.h>
#include <api/render/realtime/Handle.h>
#include <api/ui/Image.h>
#include <api/ui/Interface.h>
#include <api/ui/paint/ComponentRenderer.h>

#include <map>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include <boost/json/parse.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <entt/entt.hpp>
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace v3d::ui::documents {

/*
 * What the cases that read a ui document share: loading one written inline, a renderer that
 * needs no font, and the image resolvers an app would pass.
 */

/**
 * A ui::Interface over a document written inline, which is all a config file amounts to by
 * the time it reaches the loader.
 **/
inline boost::shared_ptr<v3d::ui::Interface> load(const std::string& document, bool* loaded) {
    const boost::shared_ptr<entt::dispatcher> dispatcher = boost::make_shared<entt::dispatcher>();
    boost::shared_ptr<v3d::ui::Interface> ui = boost::make_shared<v3d::ui::Interface>(
        boost::make_shared<v3d::event::Router>(dispatcher),
        dispatcher,
        boost::make_shared<v3d::log::Logger>());

    *loaded = ui->load(boost::json::parse(document).as_object());
    return ui;
}

/**
 * A renderer that records nothing but the geometry, since none of these cases is about
 * where a label went.
 **/
inline v3d::ui::paint::ComponentRenderer renderer() {
    return v3d::ui::paint::ComponentRenderer(
        [](std::string_view text) { return static_cast<float>(text.size()) * 10.0F; },
        [](std::string_view, const glm::vec2&, const glm::vec4&) {});
}

/**
 * What the renderer asked to be written, for the cases that are about a label.
 **/
struct Written final {
    std::string text;
    glm::vec2 pen;
    glm::vec4 colour;
};

/**
 * Hands every source the same texture, and says which sources it was asked for.
 **/
struct Uploader final {
    v3d::render::realtime::TextureHandle operator()(const std::string& source) {
        asked.push_back(source);
        return v3d::render::realtime::TextureHandle(7);
    }

    std::vector<std::string> asked;
};

/**
 * Answers the names in a sprite sheet with the one texture the sheet was uploaded to and the
 * part of it each name is, the way an app with sheets would, and nothing for anything else.
 **/
struct Sheet final {
    v3d::ui::Image operator()(const std::string& source) const {
        static const std::map<std::string, std::pair<glm::vec2, glm::vec2>> regions = {
            { "items/wood", { glm::vec2(0.0F, 0.0F), glm::vec2(0.25F, 0.5F) } },
            { "items/stone", { glm::vec2(0.25F, 0.0F), glm::vec2(0.5F, 0.5F) } },
            { "skins/center", { glm::vec2(0.5F, 0.5F), glm::vec2(0.75F, 1.0F) } },
        };
        const auto found = regions.find(source);
        if (found == regions.end()) {
            return v3d::ui::Image();
        }
        return v3d::ui::Image(v3d::render::realtime::TextureHandle(9), {.min = found->second.first, .max = found->second.second});
    }
};

/**
 * A document with two themes, "light" active. "dark" styles the base, a button and a label.
 **/
inline const char* const themedDocument = R"({
        "themes": [
            {
                "name": "dark",
                "styles": [
                    {
                        "class": "ui", "name": "default",
                        "colors": [ { "name": "panel", "value": [0.1, 0.2, 0.3, 0.5] } ],
                        "numbers": [ { "name": "bar-height", "value": 40 } ]
                    },
                    {
                        "class": "button", "name": "default", "state": "normal",
                        "numbers": [ { "name": "corner", "value": 4 } ],
                        "images": [
                            { "name": "center", "source": "skins/center.tga" },
                            { "name": "top-left", "source": "skins/tl.tga", "align": "top-left" }
                        ]
                    },
                    {
                        "class": "label", "name": "default",
                        "fonts": [ { "name": "label", "face": "Vera", "size": 18, "bold": true } ]
                    }
                ]
            },
            { "name": "light" }
        ],
        "theme": "light",
        "containers": [ { "name": "hud", "visible": true, "components": [] } ]
})";

};  // namespace v3d::ui::documents
