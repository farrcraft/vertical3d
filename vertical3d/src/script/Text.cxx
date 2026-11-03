/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#include "Text.h"

#include <api/brep/BRep.h>
#include <api/brep/Index.h>
#include <api/curve/Curves.h>
#include <vertical3d/src/scene/item/Components.h>
#include <vertical3d/src/scene/item/Curve.h>
#include <vertical3d/src/scene/item/CurveParts.h>
#include <vertical3d/src/scene/item/Item.h>
#include <vertical3d/src/scene/item/Mesh.h>
#include <vertical3d/src/scene/Scene.h>

#include "Arguments.h"

#include <algorithm>
#include <array>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>
#include <vector>

#include <boost/json.hpp>
#include <boost/shared_ptr.hpp>

namespace v3d::editor::script {

namespace {

/**
 * The letter that names each kind of part, in the order of Component.
 **/
constexpr std::array<char, 4> LETTERS{ 'v', 'e', 'f', 'h' };

/**
 * @return a value as a script writes it
 **/
std::string written(const boost::json::value& value) {
    if (value.is_double()) {
        std::array<char, 32> digits{};
        const std::to_chars_result result = std::to_chars(digits.data(), digits.data() + digits.size(), value.as_double());
        return result.ec == std::errc() ? std::string(digits.data(), result.ptr) : boost::json::serialize(value);
    }
    if (value.is_array()) {
        std::string list = "[";
        for (const boost::json::value& each : value.as_array()) {
            list += (list.size() > 1 ? ", " : "") + written(each);
        }
        return list + "]";
    }
    return boost::json::serialize(value);
}

/**
 * @return a number read from all of a text, or nothing when the text is not one
 **/
std::optional<v3d::brep::Index> index(std::string_view text) {
    v3d::brep::Index read = 0;
    const std::from_chars_result result = std::from_chars(text.data(), text.data() + text.size(), read);
    if (text.empty() || result.ec != std::errc() || result.ptr != text.data() + text.size()) {
        return std::nullopt;
    }
    return read;
}

/**
 * @return how many parts of a kind an item has, or nothing when it has none of that kind
 **/
std::optional<std::size_t> count(const scene::item::Item& item, scene::item::Component kind) {
    if (item.kind() == scene::item::Item::Kind::Mesh) {
        const v3d::brep::BRep& mesh = *static_cast<const scene::item::Mesh&>(item).geometry();
        switch (kind) {
            case scene::item::Component::Vertex:
                return mesh.vertexCount();
            case scene::item::Component::Edge:
                return mesh.edgeCount();
            case scene::item::Component::Face:
                return mesh.faceCount();
            case scene::item::Component::Handle:
                return std::nullopt;
        }
    }
    if (item.kind() == scene::item::Item::Kind::Curve) {
        const v3d::curve::Curves& curves = *static_cast<const scene::item::Curve&>(item).geometry();
        if (kind == scene::item::Component::Vertex) {
            return scene::item::pointCount(curves);
        }
        if (kind == scene::item::Component::Handle) {
            return scene::item::pointCount(curves) * 2;
        }
    }
    return std::nullopt;
}

/**
 * @return whether an item has a part
 **/
bool holds(const scene::item::Item& item, scene::item::Component kind, v3d::brep::Index part) {
    if (kind == scene::item::Component::Handle) {
        return scene::item::handleAt(*static_cast<const scene::item::Curve&>(item).geometry(), part).has_value();
    }
    return part < count(item, kind).value_or(0);
}

};  // namespace

/**
 **/
std::string path(std::string_view command) {
    std::string made;
    for (std::size_t at = 0; at < command.size(); ++at) {
        if (command.substr(at, 2) == "::") {
            made += '.';
            ++at;
        } else {
            made += command[at];
        }
    }
    return made;
}

/**
 **/
std::string line(std::string_view command, const std::vector<Parameter>& parameters, const boost::json::object& values) {
    std::string listed;
    for (const Parameter& parameter : parameters) {
        if (const boost::json::value* value = values.if_contains(parameter.name)) {
            listed += (listed.empty() ? "" : ", ") + parameter.name + ": " + written(*value);
        }
    }
    return path(command) + (listed.empty() ? "()" : "({ " + listed + " })");
}

/**
 **/
std::optional<Part> part(std::string_view text) {
    if (text.empty() || text.back() != ']') {
        return std::nullopt;
    }
    const std::size_t open = text.rfind('[');
    if (open == std::string_view::npos || open < 3 || text[open - 2] != '.') {
        return std::nullopt;
    }
    const auto letter = std::find(LETTERS.begin(), LETTERS.end(), text[open - 1]);
    if (letter == LETTERS.end()) {
        return std::nullopt;
    }
    const std::string_view run = text.substr(open + 1, text.size() - open - 2);
    const std::size_t colon = run.find(':');
    const std::optional<v3d::brep::Index> first = index(run.substr(0, colon));
    const std::optional<v3d::brep::Index> last = colon == std::string_view::npos ? first : index(run.substr(colon + 1));
    if (!first || !last || *last < *first) {
        return std::nullopt;
    }
    return Part{ .item = std::string(text.substr(0, open - 2)), .kind = static_cast<scene::item::Component>(letter - LETTERS.begin()),
        .first = *first, .last = *last };
}

/**
 **/
std::string text(const Part& part) {
    std::string run = std::to_string(part.first);
    if (part.last != part.first) {
        run += ":" + std::to_string(part.last);
    }
    return part.item + "." + LETTERS[static_cast<std::size_t>(part.kind)] + "[" + run + "]";
}

/**
 **/
std::vector<std::string> names(const scene::Items& items) {
    std::vector<std::string> named;
    named.reserve(items.size());
    for (const boost::shared_ptr<scene::item::Item>& item : items) {
        named.push_back(item->name());
    }
    return named;
}

/**
 **/
std::vector<std::string> selectedParts(const scene::Scene& scene, const scene::Items& items) {
    std::vector<std::string> parts;
    for (const boost::shared_ptr<scene::item::Item>& item : items) {
        for (std::size_t kind = 0; kind < LETTERS.size(); ++kind) {
            const std::vector<v3d::brep::Index> chosen = scene.components().of(item->id(), static_cast<scene::item::Component>(kind));
            for (std::size_t at = 0; at < chosen.size();) {
                std::size_t end = at;
                while (end + 1 < chosen.size() && chosen[end + 1] == chosen[end] + 1) {
                    ++end;
                }
                parts.push_back(text({ .item = item->name(), .kind = static_cast<scene::item::Component>(kind), .first = chosen[at], .last = chosen[end] }));
                at = end + 1;
            }
        }
    }
    return parts;
}

/**
 **/
std::optional<std::string> resolve(const scene::Scene& scene, const std::vector<std::string>& names, scene::Items* items) {
    scene::Items found;
    for (const std::string& name : names) {
        const boost::shared_ptr<scene::item::Item> item = scene.named(name);
        if (!item) {
            return "there is no item called " + name;
        }
        if (std::find(found.begin(), found.end(), item) == found.end()) {
            found.push_back(item);
        }
    }
    *items = std::move(found);
    return std::nullopt;
}

/**
 **/
std::optional<std::string> select(scene::Scene* scene, const std::vector<std::string>& parts) {
    std::vector<std::pair<boost::shared_ptr<scene::item::Item>, Part>> chosen;
    for (const std::string& each : parts) {
        const std::optional<Part> named = part(each);
        if (!named) {
            return each + " does not name parts, as Cube.f[2] or Cube.v[0:3] does";
        }
        const boost::shared_ptr<scene::item::Item> item = scene->named(named->item);
        if (!item) {
            return "there is no item called " + named->item;
        }
        if (!count(*item, named->kind)) {
            return named->item + " has no parts of the kind " + each + " names";
        }
        for (v3d::brep::Index index = named->first; index <= named->last; ++index) {
            if (!holds(*item, named->kind, index)) {
                return named->item + " has no part " + std::to_string(index) + " of the kind " + each + " names";
            }
        }
        chosen.emplace_back(item, *named);
    }
    scene->deselect();
    for (const auto& [item, named] : chosen) {
        item->selected(true);
        for (v3d::brep::Index index = named.first; index <= named.last; ++index) {
            scene->components().select(item->id(), named.kind, index, true);
        }
    }
    return std::nullopt;
}

};  // namespace v3d::editor::script
