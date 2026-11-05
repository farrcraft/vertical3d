/**
 * Vertical3D
 * Copyright(c) 2026 Joshua Farr(josh@farrcraft.com)
 **/

#pragma once

#include <api/brep/Index.h>
#include <vertical3d/src/scene/item/Components.h>
#include <vertical3d/src/scene/Scene.h>

#include <optional>
#include <string>
#include <string_view>
#include <vector>

#include "Arguments.h"

#include <boost/json.hpp>

namespace v3d::editor::script {

/**
 * @return the path a script calls a command by, its name with each "::" a dot, as
 *         "timeline::set" is timeline.set
 **/
std::string path(std::string_view command);

/**
 * The line a script writes to run a command with some values, such as
 * timeline.set({ items: ["Cube"], frame: 24 }). The values are written in the order of the
 * parameters, and a command run with none is called with nothing, as create.poly.cube().
 **/
std::string line(std::string_view command, const std::vector<Parameter>& parameters, const boost::json::object& values);

/**
 * Parts of one item that run on from one index to another, as a script names them. The text
 * is the item's name, a dot, a letter for the kind of part, and the index or the first and last
 * of a run in brackets: "Cube.f[2]" or "Cube.v[0:3]". The letters are v for a vertex or a
 * curve's control point, e for a half edge, f for a face and h for a curve's handle. The item's
 * name may hold dots and brackets, since the form is read from its end.
 **/
struct Part final {
    std::string item;
    scene::item::Component kind = scene::item::Component::Vertex;
    v3d::brep::Index first = 0;
    v3d::brep::Index last = 0;

    bool operator==(const Part& other) const = default;
};

/**
 * @return the parts a text names, or nothing when it is not in the form, or its run ends
 *         before it starts
 **/
std::optional<Part> part(std::string_view text);

/**
 * @return the text that names parts, the index alone when the run is one part long
 **/
std::string text(const Part& part);

/**
 * @return the names of a set of items, in its order
 **/
std::vector<std::string> names(const scene::Items& items);

/**
 * @return the selected parts of a set of items, as text, each kind of each item in rising runs
 **/
std::vector<std::string> selectedParts(const scene::Scene& scene, const scene::Items& items);

/**
 * Find the items a script named.
 * @param items filled with the items, in the order named, each once
 * @return why one could not be found, or nothing when every one was
 **/
std::optional<std::string> resolve(const scene::Scene& scene, const std::vector<std::string>& names, scene::Items* items);

/**
 * Select the parts a script named, and nothing else of any item. Nothing changes when one
 * cannot be selected.
 * @return why one could not be selected: its text is not in the form, there is no such item,
 *         the item has no parts of that kind, or no part of that index
 **/
std::optional<std::string> select(scene::Scene* scene, const std::vector<std::string>& parts);

};  // namespace v3d::editor::script
